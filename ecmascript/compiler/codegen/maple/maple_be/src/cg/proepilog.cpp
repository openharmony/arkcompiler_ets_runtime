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

#include "proepilog.h"
#include "cgfunc.h"
#include "cg.h"

namespace maplebe {
using namespace maple;

Insn *GenProEpilog::InsertCFIDefCfaOffset(int32 &cfiOffset, Insn &insertAfter)
{
    if (!cgFunc.GenCfi()) {
        return &insertAfter;
    }
    cfiOffset = AddtoOffsetFromCFA(cfiOffset);
    Insn &cfiInsn = cgFunc.GetInsnBuilder()
                        ->BuildCfiInsn(cfi::OP_CFI_def_cfa_offset)
                        .AddOpndChain(cgFunc.CreateCfiImmOperand(cfiOffset, k64BitSize));
    Insn *newIPoint = cgFunc.GetCurBB()->InsertInsnAfter(insertAfter, cfiInsn);
    cgFunc.SetDbgCallFrameOffset(cfiOffset);
    return newIPoint;
}

/* there are two stack protector:
 * 1. stack protector all: for all function
 * 2. stack protector strong: for some functon that
 *   <1> invoke alloca functon;
 *   <2> use stack address;
 *   <3> callee use return stack slot;
 *   <4> local symbol is vector type;
 * */
void GenProEpilog::NeedStackProtect()
{
    DEBUG_ASSERT(stackProtect == false, "no stack protect default");
    CG *currCG = cgFunc.GetCG();
    if (currCG->IsStackProtectorAll()) {
        stackProtect = true;
        return;
    }

    if (!currCG->IsStackProtectorStrong()) {
        return;
    }

    if (cgFunc.HasAlloca()) {
        stackProtect = true;
        return;
    }

    /* check if function use stack address or callee function return stack slot */
    auto stackProtectInfo = cgFunc.GetStackProtectInfo();
    if ((stackProtectInfo & kAddrofStack) != 0 || (stackProtectInfo & kRetureStackSlot) != 0) {
        stackProtect = true;
        return;
    }

    /* check if local symbol is vector type */
    auto &mirFunction = cgFunc.GetFunction();
    uint32 symTabSize = static_cast<uint32>(mirFunction.GetSymTab()->GetSymbolTableSize());
    for (uint32 i = 0; i < symTabSize; ++i) {
        MIRSymbol *symbol = mirFunction.GetSymTab()->GetSymbolFromStIdx(i);
        if (symbol == nullptr || symbol->GetStorageClass() != kScAuto || symbol->IsDeleted()) {
            continue;
        }
        TyIdx tyIdx = symbol->GetTyIdx();
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
        if (type->GetKind() == kTypeArray) {
            stackProtect = true;
            return;
        }

        if (type->IsStructType() && IncludeArray(*type)) {
            stackProtect = true;
            return;
        }
    }
}

bool GenProEpilog::IncludeArray(const MIRType &type) const
{
    DEBUG_ASSERT(type.IsStructType(), "agg must be one of class/struct/union");
    auto &structType = static_cast<const MIRStructType &>(type);
    /* all elements of struct. */
    auto num = static_cast<uint8>(structType.GetFieldsSize());
    for (uint32 i = 0; i < num; ++i) {
        MIRType *elemType = structType.GetElemType(i);
        if (elemType->GetKind() == kTypeArray) {
            return true;
        }
        if (elemType->IsStructType() && IncludeArray(*elemType)) {
            return true;
        }
    }
    return false;
}

bool CgGenProEpiLog::PhaseRun(maplebe::CGFunc &f)
{
    GenProEpilog *genPE = nullptr;
    genPE = f.GetCG()->CreateGenProEpilog(f, *GetPhaseMemPool(), ApplyTempMemPool());
    genPE->Run();
    return false;
}
} /* namespace maplebe */
