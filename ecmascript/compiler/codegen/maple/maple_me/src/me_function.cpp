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

#include "me_function.h"
#include <iostream>
#include <functional>
#include "ssa_mir_nodes.h"
#include "me_cfg.h"
#include "mir_lower.h"
#include "mir_builder.h"
#include "constantfold.h"
#include "me_irmap.h"
#include "me_ssa_update.h"

namespace maple {
#if DEBUG
MIRModule *globalMIRModule = nullptr;
MeFunction *globalFunc = nullptr;
MeIRMap *globalIRMap = nullptr;
SSATab *globalSSATab = nullptr;
#endif
void MeFunction::PartialInit()
{
    theCFG = nullptr;
    irmap = nullptr;
    regNum = 0;
    hasEH = false;
    ConstantFold cf(mirModule);
    (void)cf.Simplify(mirFunc->GetBody());
}

void MeFunction::Release()
{
    ReleaseVersMemory();
    if (preMeMp) {
        memPoolCtrler.DeleteMemPool(preMeMp);
    }
    preMeMp = nullptr;
}

void MeFunction::Verify() const
{
    CHECK_FATAL(theCFG != nullptr, "theCFG is null");
    theCFG->Verify();
    theCFG->VerifyLabels();
}

/* create label for bb */
LabelIdx MeFunction::GetOrCreateBBLabel(BB &bb)
{
    LabelIdx label = bb.GetBBLabel();
    if (label != 0) {
        return label;
    }
    label = mirModule.CurFunction()->GetLabelTab()->CreateLabelWithPrefix('m');
    mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(label);
    bb.SetBBLabel(label);
    theCFG->SetLabelBBAt(label, &bb);
    return label;
}
}  // namespace maple
