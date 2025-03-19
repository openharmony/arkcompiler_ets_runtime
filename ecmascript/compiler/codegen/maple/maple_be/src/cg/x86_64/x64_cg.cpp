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

#include "x64_cg.h"
namespace maplebe {

using namespace x64;

#define DEFINE_MOP(...) {__VA_ARGS__},
const InsnDesc X64CG::kMd[kMopLast] = {
#include "abstract_mmir.def"
#include "x64_md.def"
};
#undef DEFINE_MOP

void X64CG::EnrollTargetPhases(maple::MaplePhaseManager *pm) const {
#include "x64_phases.def"
}

CGFunc *X64CG::CreateCGFunc(MIRModule &mod, MIRFunction &mirFunc, BECommon &bec, MemPool &memPool,
                            StackMemPool &stackMp, MapleAllocator &mallocator, uint32 funcId)
{
    return memPool.New<X64CGFunc>(mod, *this, mirFunc, bec, memPool, stackMp, mallocator, funcId);
}

bool X64CG::IsEffectiveCopy(Insn &insn) const
{
    return false;
}
bool X64CG::IsTargetInsn(MOperator mOp) const
{
    return (mOp >= MOP_movb_r_r && mOp <= MOP_pseudo_ret_int);
}
bool X64CG::IsClinitInsn(MOperator mOp) const
{
    return false;
}
bool X64CG::IsPseudoInsn(MOperator mOp) const
{
    return false;
}

PhiOperand &X64CG::CreatePhiOperand(MemPool &mp, MapleAllocator &mAllocator)
{
    CHECK_FATAL(false, "NIY");
    PhiOperand *a = nullptr;
    return *a;
}

void X64CG::DumpTargetOperand(Operand &opnd, const OpndDesc &opndDesc) const
{
    X64OpndDumpVisitor visitor(opndDesc);
    opnd.Accept(visitor);
}

bool X64CG::IsExclusiveFunc(MIRFunction &mirFunc)
{
    return false;
}

#ifdef ARK_LITECG_DEBUG
/* Used for GCTIB pattern merging */
std::string X64CG::FindGCTIBPatternName(const std::string &name) const
{
    return "";
}
#endif
}  // namespace maplebe
