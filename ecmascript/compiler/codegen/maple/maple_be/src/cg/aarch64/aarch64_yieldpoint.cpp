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

#include "aarch64_yieldpoint.h"
#include "aarch64_cgfunc.h"

namespace maplebe {
using namespace maple;

void AArch64YieldPointInsertion::Run()
{
    InsertYieldPoint();
}

void AArch64YieldPointInsertion::InsertYieldPoint()
{
    AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc *>(cgFunc);

    /*
     * do not insert yieldpoint in function that not saved X30 into stack,
     * because X30 will be changed after yieldpoint is taken.
     */
    if (!aarchCGFunc->GetHasProEpilogue()) {
        DEBUG_ASSERT(aarchCGFunc->GetYieldPointInsn() != nullptr, "the entry yield point has been inserted");
        aarchCGFunc->GetYieldPointInsn()->GetBB()->RemoveInsn(*aarchCGFunc->GetYieldPointInsn());
        return;
    }
    /* skip if no GetFirstbb(). */
    if (aarchCGFunc->GetFirstBB() == nullptr) {
        return;
    }
    /*
     * The yield point in the entry of the GetFunction() is inserted just after the initialization
     * of localrefvars in HandleRCCall.
     * for BBs after firstbb.
     */
    for (BB *bb = aarchCGFunc->GetFirstBB()->GetNext(); bb != nullptr; bb = bb->GetNext()) {
        // insert a yieldpoint at loop header bb
        auto *loop = loopInfo.GetBBLoopParent(bb->GetId());
        if (loop != nullptr && loop->GetHeader().GetId() == bb->GetId()) {
            aarchCGFunc->GetDummyBB()->ClearInsns();
            aarchCGFunc->GenerateYieldpoint(*aarchCGFunc->GetDummyBB());
            bb->InsertAtBeginning(*aarchCGFunc->GetDummyBB());
        }
    }
}
} /* namespace maplebe */
