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

#include "emit.h"

namespace maplebe {
using namespace maple;

uint32 VregInfo::virtualRegCount = kBaseVirtualRegNO;
uint32 VregInfo::maxRegCount = 0;
std::vector<VirtualRegNode> VregInfo::vRegTable;
std::unordered_map<regno_t, RegOperand *> VregInfo::vRegOperandTable;
/*  There are two builders, cgfunc builder (original code selector) and irbuilder (abstract).
 *  This is to prevent conflict between the two for VregInfo as for arm64 both co-exists.
 *  When switching to irbuilder completely, then this bool can go away.
 */
bool VregInfo::initialized = false;

void Globals::SetTarget(CG &target)
{
    cg = &target;
}
const CG *Globals::GetTarget() const
{
    DEBUG_ASSERT(cg, " set target info please ");
    return cg;
}

CGFunc *CG::currentCGFunction = nullptr;
std::map<MIRFunction *, std::pair<LabelIdx, LabelIdx>> CG::funcWrapLabels;

CG::~CG()
{
    Emit([](Emitter *emitter) {
        emitter->CloseOutput();
    });
    delete memPool;
    memPool = nullptr;
    mirModule = nullptr;
    emitters.clear();
    currentCGFunction = nullptr;
    dbgTraceEnter = nullptr;
    dbgTraceExit = nullptr;
    dbgFuncProfile = nullptr;
}

void CG::EmitAllEmitters(const std::function<void(Emitter *)>& cb) const
{
    DEBUG_ASSERT(!emitters.empty(), "Emitter were not set");
    DEBUG_ASSERT(emitters.size() <= 2U, "Emitters number isn't correct");
    for (auto emitter: emitters) {
        cb(emitter);
    }
}

void CG::EmitAsmEmitters(const std::function<void(Emitter *)>& cb) const
{
    if (emitters.size() == 2U) {
        cb(emitters[1]);
    }
}

void CG::EmitObjEmitters(const std::function<void(Emitter *)>& cb) const
{
    DEBUG_ASSERT(!emitters.empty(), "ObjEmmiter wasn't set");
    cb(emitters[0]);
}

} /* namespace maplebe */
