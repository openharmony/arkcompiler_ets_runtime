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

#ifndef MAPLEBE_INCLUDE_CG_CRITICAL_EDGE_H
#define MAPLEBE_INCLUDE_CG_CRITICAL_EDGE_H

#include "cgbb.h"
#include "insn.h"

namespace maplebe {
class CriticalEdge {
public:
    CriticalEdge(CGFunc &func, MemPool &mem) : cgFunc(&func), alloc(&mem), criticalEdges(alloc.Adapter()) {}

    ~CriticalEdge() = default;

    void CollectCriticalEdges();
    void SplitCriticalEdges();

private:
    CGFunc *cgFunc;
    MapleAllocator alloc;
    MapleVector<std::pair<BB *, BB *>> criticalEdges;
};

MAPLE_FUNC_PHASE_DECLARE(CgCriticalEdge, maplebe::CGFunc)
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_CRITICAL_EDGE_H */
