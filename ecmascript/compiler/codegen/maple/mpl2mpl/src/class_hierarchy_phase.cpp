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

#include "class_hierarchy_phase.h"
namespace maple {
bool M2MKlassHierarchy::PhaseRun(maple::MIRModule &m)
{
    kh = GetPhaseMemPool()->New<KlassHierarchy>(&m, GetPhaseMemPool());
    KlassHierarchy::traceFlag = Options::dumpPhase.compare(PhaseName()) == 0;
    kh->BuildHierarchy();
#if MIR_JAVA
    if (!Options::skipVirtualMethod) {
        kh->CountVirtualMethods();
    }
#else
    kh->CountVirtualMethods();
#endif
    if (KlassHierarchy::traceFlag) {
        kh->Dump();
    }
    return true;
}
}  // namespace maple
