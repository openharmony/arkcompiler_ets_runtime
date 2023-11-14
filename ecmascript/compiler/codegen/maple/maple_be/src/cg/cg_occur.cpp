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

#include "cg_occur.h"
#include "cg_pre.h"

/* The methods associated with the data structures that represent occurrences and work candidates for PRE */
namespace maplebe {
/* return if this occur dominate occ */
bool CgOccur::IsDominate(DomAnalysis &dom, CgOccur &occ)
{
    return dom.Dominate(*GetBB(), *occ.GetBB());
}

/* compute bucket index for the work candidate in workCandHashTable */
uint32 PreWorkCandHashTable::ComputeWorkCandHashIndex(const Operand &opnd)
{
    uint32 hashIdx = static_cast<uint32>(reinterpret_cast<uint64>(&opnd) >> k4ByteSize);
    return hashIdx % workCandHashLength;
}

uint32 PreWorkCandHashTable::ComputeStmtWorkCandHashIndex(const Insn &insn)
{
    uint32 hIdx = (static_cast<uint32>(insn.GetMachineOpcode())) << k3ByteSize;
    return hIdx % workCandHashLength;
}
}  // namespace maplebe
