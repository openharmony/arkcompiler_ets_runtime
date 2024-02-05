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

#include "insn.h"
#include "isa.h"
#include "cg.h"
namespace maplebe {
bool Insn::IsMachineInstruction() const
{
    return md && md->IsPhysicalInsn() && Globals::GetInstance()->GetTarget()->IsTargetInsn(mOp);
}
/* phi is not physical insn */
bool Insn::IsPhi() const
{
    return md ? md->IsPhi() : false;
}
bool Insn::IsLoad() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsLoad();
}
bool Insn::IsStore() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsStore();
}
bool Insn::IsMove() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsMove();
}
bool Insn::IsBranch() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsBranch();
}
bool Insn::IsCondBranch() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsCondBranch();
}
bool Insn::IsUnCondBranch() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsUnCondBranch();
}
bool Insn::IsBasicOp() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsBasicOp();
}
bool Insn::IsConversion() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsConversion();
}
bool Insn::IsUnaryOp() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsUnaryOp();
}
bool Insn::IsShift() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsShift();
}
bool Insn::IsCall() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsCall();
}
bool Insn::IsSpecialCall() const
{
    return md ? md->IsSpecialCall() : false;
}
bool Insn::IsTailCall() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsTailCall();
}
bool Insn::IsAsmInsn() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsInlineAsm();
}
bool Insn::IsDMBInsn() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsDMB();
}
bool Insn::IsAtomic() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsAtomic();
}
bool Insn::IsVolatile() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsVolatile();
}
bool Insn::IsMemAccessBar() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsMemAccessBar();
}
bool Insn::IsMemAccess() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsMemAccess();
}
bool Insn::CanThrow() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->CanThrow();
}
bool Insn::IsVectorOp() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsVectorOp();
}
bool Insn::HasLoop() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->HasLoop();
}
uint32 Insn::GetLatencyType() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->GetLatencyType();
}
uint32 Insn::GetAtomicNum() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->GetAtomicNum();
}
bool Insn::IsSpecialIntrinsic() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsSpecialIntrinsic();
}
bool Insn::IsLoadPair() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsLoadPair();
}
bool Insn::IsStorePair() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsStorePair();
}
bool Insn::IsLoadStorePair() const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->IsLoadStorePair();
}
bool Insn::IsLoadLabel() const
{
    return md->IsLoad() && GetOperand(kInsnSecondOpnd).GetKind() == Operand::kOpdBBAddress;
}
bool Insn::OpndIsDef(uint32 id) const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->GetOpndDes(id)->IsDef();
}
bool Insn::OpndIsUse(uint32 id) const
{
    DEBUG_ASSERT(md, " set insnDescription for insn ");
    return md->GetOpndDes(id)->IsUse();
}
bool Insn::IsClinit() const
{
    return Globals::GetInstance()->GetTarget()->IsClinitInsn(mOp);
}
bool Insn::IsComment() const
{
    return mOp == abstract::MOP_comment && !md->IsPhysicalInsn();
}

bool Insn::IsImmaterialInsn() const
{
    return IsComment();
}

bool Insn::IsPseudo() const
{
    return md && md->IsPhysicalInsn() && Globals::GetInstance()->GetTarget()->IsPseudoInsn(mOp);
}

Operand *Insn::GetMemOpnd() const
{
    for (uint32 i = 0; i < opnds.size(); ++i) {
        Operand &opnd = GetOperand(i);
        if (opnd.IsMemoryAccessOperand()) {
            return &opnd;
        }
    }
    return nullptr;
}

uint32 Insn::GetMemOpndIdx() const
{
    uint32 opndIdx = kInsnMaxOpnd;
    for (uint32 i = 0; i < static_cast<uint32>(opnds.size()); ++i) {
        Operand &opnd = GetOperand(i);
        if (opnd.IsMemoryAccessOperand()) {
            return i;
        }
    }
    return opndIdx;
}
void Insn::SetMemOpnd(MemOperand *memOpnd)
{
    for (uint32 i = 0; i < static_cast<uint32>(opnds.size()); ++i) {
        Operand &opnd = GetOperand(i);
        if (opnd.IsMemoryAccessOperand()) {
            SetOperand(i, *memOpnd);
            return;
        }
    }
}

bool Insn::IsRegDefined(regno_t regNO) const
{
    return GetDefRegs().count(regNO);
}

std::set<uint32> Insn::GetDefRegs() const
{
    std::set<uint32> defRegNOs;
    size_t opndNum = opnds.size();
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = GetOperand(i);
        auto *regProp = md->opndMD[i];
        bool isDef = regProp->IsDef();
        if (!isDef && !opnd.IsMemoryAccessOperand()) {
            continue;
        }
        if (opnd.IsList()) {
            for (auto *op : static_cast<ListOperand &>(opnd).GetOperands()) {
                DEBUG_ASSERT(op != nullptr, "invalid operand in list operand");
                defRegNOs.emplace(op->GetRegisterNumber());
            }
        } else if (opnd.IsMemoryAccessOperand()) {
            auto &memOpnd = static_cast<MemOperand &>(opnd);
            RegOperand *base = memOpnd.GetBaseRegister();
            if (base != nullptr) {
                if (memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi &&
                    (memOpnd.IsPostIndexed() || memOpnd.IsPreIndexed())) {
                    DEBUG_ASSERT(!defRegNOs.count(base->GetRegisterNumber()), "duplicate def in one insn");
                    defRegNOs.emplace(base->GetRegisterNumber());
                }
            }
        } else if (opnd.IsConditionCode() || opnd.IsRegister()) {
            defRegNOs.emplace(static_cast<RegOperand &>(opnd).GetRegisterNumber());
        }
    }
    return defRegNOs;
}

#if DEBUG
void Insn::Check() const
{
    if (!md) {
        CHECK_FATAL(false, " need machine description for target insn ");
    }
    /* check if the number of operand(s) matches */
    uint32 insnOperandSize = GetOperandSize();
    if (insnOperandSize != md->GetOpndMDLength()) {
        CHECK_FATAL(false, " the number of operands in instruction does not match machine description ");
    }
    /* check if the type of each operand  matches */
    for (uint32 i = 0; i < insnOperandSize; ++i) {
        Operand &opnd = GetOperand(i);
        auto *opndDesc = md->GetOpndDes(i);
        if (opnd.IsImmediate()) {
            CHECK_FATAL(opndDesc->IsImm(), "operand type does not match machine description!");
        } else {
            CHECK_FATAL(opnd.GetKind() == opndDesc->GetOperandType(),
                        "operand type does not match machine description!");
        }
    }
}
#endif

Insn *Insn::Clone(MemPool &memPool) const
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *Insn::GetCallTargetOperand() const
{
    DEBUG_ASSERT(IsCall() || IsTailCall(), "should be call");
    return &GetOperand(kInsnFirstOpnd);
}

ListOperand *Insn::GetCallArgumentOperand()
{
    DEBUG_ASSERT(IsCall(), "should be call");
    DEBUG_ASSERT(GetOperand(1).IsList(), "should be list");
    return &static_cast<ListOperand &>(GetOperand(kInsnSecondOpnd));
}

void Insn::CommuteOperands(uint32 dIndex, uint32 sIndex)
{
    Operand *tempCopy = opnds[sIndex];
    opnds[sIndex] = opnds[dIndex];
    opnds[dIndex] = tempCopy;
}

uint32 Insn::GetBothDefUseOpnd() const
{
    size_t opndNum = opnds.size();
    uint32 opndIdx = kInsnMaxOpnd;
    if (md->GetAtomicNum() > 1) {
        return opndIdx;
    }
    for (uint32 i = 0; i < opndNum; ++i) {
        auto *opndProp = md->GetOpndDes(i);
        if (opndProp->IsRegUse() && opndProp->IsDef()) {
            DEBUG_ASSERT(opndIdx == kInsnMaxOpnd, "Do not support yet");
            opndIdx = i;
        }
        if (opnds[i]->IsMemoryAccessOperand()) {
            auto *MemOpnd = static_cast<MemOperand *>(opnds[i]);
            if (!MemOpnd->IsIntactIndexed()) {
                DEBUG_ASSERT(opndIdx == kInsnMaxOpnd, "Do not support yet");
                opndIdx = i;
            }
        }
    }
    return opndIdx;
}

uint32 Insn::GetMemoryByteSize() const
{
    DEBUG_ASSERT(IsMemAccess(), "must be memory access insn");
    uint32 res = 0;
    for (size_t i = 0; i < opnds.size(); ++i) {
        if (md->GetOpndDes(i)->GetOperandType() == Operand::kOpdMem) {
            res = md->GetOpndDes(i)->GetSize();
        }
    }
    DEBUG_ASSERT(res, "cannot access empty memory");
    if (IsLoadStorePair()) {
        res = res << 1;
    }
    res = res >> k8BitShift;
    return res;
}

bool Insn::ScanReg(regno_t regNO) const
{
    uint32 opndNum = GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = GetOperand(i);
        if (opnd.IsList()) {
            auto &listOpnd = static_cast<ListOperand &>(opnd);
            for (auto listElem : listOpnd.GetOperands()) {
                auto *regOpnd = static_cast<RegOperand *>(listElem);
                DEBUG_ASSERT(regOpnd != nullptr, "parameter operand must be RegOperand");
                if (regNO == regOpnd->GetRegisterNumber()) {
                    return true;
                }
            }
        } else if (opnd.IsMemoryAccessOperand()) {
            auto &memOpnd = static_cast<MemOperand &>(opnd);
            RegOperand *base = memOpnd.GetBaseRegister();
            RegOperand *index = memOpnd.GetIndexRegister();
            if ((base != nullptr && base->GetRegisterNumber() == regNO) ||
                (index != nullptr && index->GetRegisterNumber() == regNO)) {
                return true;
            }
        } else if (opnd.IsRegister()) {
            if (static_cast<RegOperand &>(opnd).GetRegisterNumber() == regNO) {
                return true;
            }
        }
    }
    return false;
}

bool Insn::MayThrow() const
{
    if (md->IsMemAccess() && !IsLoadLabel()) {
        auto *memOpnd = static_cast<MemOperand *>(GetMemOpnd());
        DEBUG_ASSERT(memOpnd != nullptr, "CG invalid memory operand.");
        if (memOpnd->IsStackMem()) {
            return false;
        }
    }
    return md->CanThrow();
}

void Insn::SetMOP(const InsnDesc &idesc)
{
    mOp = idesc.GetOpc();
    md = &idesc;
}

void Insn::Dump() const
{
    DEBUG_ASSERT(md != nullptr, "md should not be nullptr");
    LogInfo::MapleLogger() << "< " << GetId() << " > ";
    LogInfo::MapleLogger() << md->name << "(" << mOp << ")";

    for (uint32 i = 0; i < GetOperandSize(); ++i) {
        Operand &opnd = GetOperand(i);
        LogInfo::MapleLogger() << " (opnd" << i << ": ";
        Globals::GetInstance()->GetTarget()->DumpTargetOperand(opnd, *md->GetOpndDes(i));
        LogInfo::MapleLogger() << ")";
    }

    if (IsVectorOp()) {
        auto *vInsn = static_cast<const VectorInsn *>(this);
        if (vInsn->GetNumOfRegSpec() != 0) {
            LogInfo::MapleLogger() << " (vecSpec: " << vInsn->GetNumOfRegSpec() << ")";
        }
    }
    if (stackMap != nullptr) {
        const auto &deoptVreg2Opnd = stackMap->GetDeoptInfo().GetDeoptBundleInfo();
        if (!deoptVreg2Opnd.empty()) {
            LogInfo::MapleLogger() << "  (deopt: ";
            bool isFirstElem = true;
            for (const auto &elem : deoptVreg2Opnd) {
                if (!isFirstElem) {
                    LogInfo::MapleLogger() << ", ";
                } else {
                    isFirstElem = false;
                }
                LogInfo::MapleLogger() << elem.first << ":";
                elem.second->Dump();
            }
            LogInfo::MapleLogger() << ")";
        }
    }
    LogInfo::MapleLogger() << "\n";
}

VectorRegSpec *VectorInsn::GetAndRemoveRegSpecFromList()
{
    if (regSpecList.size() == 0) {
        VectorRegSpec *vecSpec = CG::GetCurCGFuncNoConst()->GetMemoryPool()->New<VectorRegSpec>();
        return vecSpec;
    }
    VectorRegSpec *ret = regSpecList.back();
    regSpecList.pop_back();
    return ret;
}
}  // namespace maplebe
