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

#include "x64_emitter.h"
#include "x64_cgfunc.h"
#include "x64_cg.h"
#include "insn.h"

#define __ assmbler.

namespace {
using namespace maple;

DBGDieAttr *LFindAttribute(MapleVector<DBGDieAttr *> &vec, DwAt key)
{
    for (DBGDieAttr *at : vec) {
        if (at->GetDwAt() == key) {
            return at;
        }
    }
    return nullptr;
}

DBGAbbrevEntry *LFindAbbrevEntry(MapleVector<DBGAbbrevEntry *> &abbvec, unsigned int key)
{
    for (DBGAbbrevEntry *daie : abbvec) {
        if (!daie) {
            continue;
        }
        if (daie->GetAbbrevId() == key) {
            return daie;
        }
    }
    DEBUG_ASSERT(0, "");
    return nullptr;
}

bool LShouldEmit(unsigned int dwform)
{
    return dwform != static_cast<uint32>(DW_FORM_flag_present);
}

DBGDie *LFindChildDieWithName(DBGDie &die, DwTag tag, const GStrIdx key)
{
    for (DBGDie *c : die.GetSubDieVec()) {
        if (c->GetTag() != tag) {
            continue;
        }
        for (DBGDieAttr *a : c->GetAttrVec()) {
            if ((a->GetDwAt() == static_cast<uint32>(DW_AT_name)) &&
                ((a->GetDwForm() == static_cast<uint32>(DW_FORM_string) ||
                  a->GetDwForm() == static_cast<uint32>(DW_FORM_strp)) &&
                 a->GetId() == key.GetIdx())) {
                return c;
            }
            if ((a->GetDwAt() == static_cast<uint32>(DW_AT_name)) &&
                (!((a->GetDwForm() == static_cast<uint32>(DW_FORM_string) ||
                    a->GetDwForm() == static_cast<uint32>(DW_FORM_strp)) &&
                   a->GetId() == key.GetIdx()))) {
                break;
            }
        }
    }
    return nullptr;
}

/* GetDwOpName(unsigned n) */
#define TOSTR(s) #s
const std::string GetDwOpName(unsigned n)
{
    switch (n) {
#define HANDLE_DW_OP(ID, NAME, VERSION, VENDOR) \
    case DW_OP_##NAME:                          \
        return TOSTR(DW_OP_##NAME)
        case DW_OP_hi_user:
            return "DW_OP_hi_user";
        default:
            return nullptr;
    }
}
}  // namespace

using namespace std;
using namespace assembler;

namespace maplebe {
uint8 X64Emitter::GetSymbolAlign(const MIRSymbol &mirSymbol, bool isComm)
{
    uint8 alignInByte = mirSymbol.GetAttrs().GetAlignValue();
    MIRTypeKind kind = mirSymbol.GetType()->GetKind();
    if (isComm) {
        MIRStorageClass storage = mirSymbol.GetStorageClass();
        if (((kind == kTypeStruct) || (kind == kTypeClass) || (kind == kTypeArray) || (kind == kTypeUnion)) &&
            ((storage == kScGlobal) || (storage == kScPstatic) || (storage == kScFstatic)) &&
            alignInByte < kSizeOfPTR) {
            alignInByte = kQ;
            return alignInByte;
        }
    }
    if (alignInByte == 0) {
        if (kind == kTypeStruct || kind == kTypeClass || kind == kTypeArray || kind == kTypeUnion) {
            return alignInByte;
        } else {
            alignInByte = Globals::GetInstance()->GetBECommon()->GetTypeAlign(mirSymbol.GetType()->GetTypeIndex());
        }
    }
    return alignInByte;
}

uint64 X64Emitter::GetSymbolSize(const TyIdx typeIndex)
{
    uint64 sizeInByte = Globals::GetInstance()->GetBECommon()->GetTypeSize(typeIndex);
    return sizeInByte;
}

Reg X64Emitter::TransferReg(Operand *opnd) const
{
    RegOperand *v = static_cast<RegOperand *>(opnd);
    /* check whether this reg is still virtual */
    CHECK_FATAL(v->IsPhysicalRegister(), "register is still virtual or reg num is 0");

    uint8 regType = -1;
    switch (v->GetSize()) {
        case k8BitSize:
            regType = v->IsHigh8Bit() ? X64CG::kR8HighList : X64CG::kR8LowList;
            break;
        case k16BitSize:
            regType = X64CG::kR16List;
            break;
        case k32BitSize:
            regType = X64CG::kR32List;
            break;
        case k64BitSize:
            regType = X64CG::kR64List;
            break;
        default:
            FATAL(kLncFatal, "unkown reg size");
            break;
    }
    Reg reg = kRegArray[regType][v->GetRegisterNumber()];
    return reg;
}

pair<int64, bool> X64Emitter::TransferImm(Operand *opnd)
{
    ImmOperand *v = static_cast<ImmOperand *>(opnd);
    if (v->GetKind() == Operand::kOpdStImmediate) {
        uint32 symIdx = v->GetSymbol()->GetNameStrIdx().get();
        const string &symName = v->GetName();
        __ StoreNameIntoSymMap(symIdx, symName);
        return pair<int64, bool>(symIdx, true);
    } else {
        return pair<int64, bool>(v->GetValue(), false);
    }
}

Mem X64Emitter::TransferMem(Operand *opnd, uint32 funcUniqueId)
{
    MemOperand *v = static_cast<MemOperand *>(opnd);
    Mem mem;
    mem.size = v->GetSize();
    if (v->GetOffsetOperand() != nullptr) {
        ImmOperand *ofset = v->GetOffsetOperand();
        if (ofset->GetKind() == Operand::kOpdStImmediate) {
            string symbolName = ofset->GetName();
            const MIRSymbol *symbol = ofset->GetSymbol();

            MIRStorageClass storageClass = symbol->GetStorageClass();
            bool isLocalVar = ofset->GetSymbol()->IsLocal();
            if (storageClass == kScPstatic && isLocalVar) {
                symbolName.append(to_string(funcUniqueId));
            }

            int64 symIdx;
            /* 2 : if it is a bb label, the second position in symbolName is '.' */
            if (symbolName.size() > 2 && symbolName[2] == '.') {
                string delimiter = "__";
                size_t pos = symbolName.find(delimiter);
                uint32 itsFuncUniqueId =
                    pos > 3 ? stoi(symbolName.substr(3, pos)) : 0; /* 3: index starts after ".L." */
                uint32 labelIdx = stoi(symbolName.substr(pos + 2, symbolName.length())); /* 2: delimiter.length() */
                symIdx = CalculateLabelSymIdx(itsFuncUniqueId, labelIdx);
            } else {
                symIdx = symbol->GetNameStrIdx().get();
            }
            __ StoreNameIntoSymMap(symIdx, symbolName);
            mem.disp.first = symIdx;
        }
        if (ofset->GetValue() != 0) {
            mem.disp.second = ofset->GetValue();
        }
    }
    if (v->GetBaseRegister() != nullptr) {
        if (v->GetIndexRegister() != nullptr && v->GetBaseRegister()->GetRegisterNumber() == x64::RBP) {
            mem.base = ERR;
        } else {
            mem.base = TransferReg(v->GetBaseRegister());
        }
    }
    if (v->GetIndexRegister() != nullptr) {
        mem.index = TransferReg(v->GetIndexRegister());
        uint8 s = static_cast<uint8>(v->GetScaleOperand()->GetValue());
        /* 1, 2, 4, 8: allowed range for s */
        CHECK_FATAL(s == 1 || s == 2 || s == 4 || s == 8, "mem.s is not 1, 2, 4, or 8");
        mem.s = s;
    }
    mem.SetMemType();
    return mem;
}

int64 X64Emitter::TransferLabel(Operand *opnd, uint32 funcUniqueId)
{
    LabelOperand *v = static_cast<LabelOperand *>(opnd);
    int64 labelSymIdx = CalculateLabelSymIdx(funcUniqueId, v->GetLabelIndex());
    __ StoreNameIntoSymMap(labelSymIdx, v->GetParentFunc());
    return labelSymIdx;
}

uint32 X64Emitter::TransferFuncName(Operand *opnd)
{
    FuncNameOperand *v = static_cast<FuncNameOperand *>(opnd);
    uint32 funcSymIdx = v->GetFunctionSymbol()->GetNameStrIdx().get();
    __ StoreNameIntoSymMap(funcSymIdx, v->GetName());
    return funcSymIdx;
}

void X64Emitter::EmitInsn(Insn &insn, uint32 funcUniqueId)
{
#if DEBUG
    insn.Check();
#endif

    MOperator mop = insn.GetMachineOpcode();
    const InsnDesc &curMd = X64CG::kMd[mop];
    uint32 opndNum = curMd.GetOpndMDLength(); /* Get operands Number */

    /* Get operand(s) */
    Operand *opnd0 = nullptr;
    Operand *opnd1 = nullptr;
    if (opndNum > 0) {
        opnd0 = &insn.GetOperand(0);
        if (opndNum > 1) {
            opnd1 = &insn.GetOperand(1);
        }
    }

    switch (mop) {
        /* mov */
        case x64::MOP_movb_r_r:
            __ Mov(kB, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movw_r_r:
            __ Mov(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movl_r_r:
            __ Mov(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movq_r_r:
            __ Mov(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movb_m_r:
            __ Mov(kB, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_movw_m_r:
            __ Mov(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_movl_m_r:
            __ Mov(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_movq_m_r:
            __ Mov(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_movb_i_r:
            __ Mov(kB, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movw_i_r:
            __ Mov(kW, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movl_i_r:
            __ Mov(kL, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movq_i_r:
            __ Mov(kQ, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movb_i_m:
            __ Mov(kB, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_movw_i_m:
            __ Mov(kW, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_movl_i_m:
            __ Mov(kL, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_movb_r_m:
            __ Mov(kB, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_movw_r_m:
            __ Mov(kW, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_movl_r_m:
            __ Mov(kL, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_movq_r_m:
            __ Mov(kQ, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        /* movzx */
        case x64::MOP_movzbw_r_r:
            __ MovZx(kB, kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movzbl_r_r:
            __ MovZx(kB, kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movzbq_r_r:
            __ MovZx(kB, kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movzwl_r_r:
            __ MovZx(kW, kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movzwq_r_r:
            __ MovZx(kW, kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movzbw_m_r:
            __ MovZx(kB, kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_movzbl_m_r:
            __ MovZx(kB, kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_movzbq_m_r:
            __ MovZx(kB, kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_movzwl_m_r:
            __ MovZx(kW, kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_movzwq_m_r:
            __ MovZx(kW, kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        /* movsx */
        case x64::MOP_movsbw_r_r:
            __ MovSx(kB, kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movsbl_r_r:
            __ MovSx(kB, kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movsbq_r_r:
            __ MovSx(kB, kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movswl_r_r:
            __ MovSx(kW, kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movswq_r_r:
            __ MovSx(kW, kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movslq_r_r:
            __ MovSx(kL, kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movsbw_m_r:
            __ MovSx(kB, kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_movsbl_m_r:
            __ MovSx(kB, kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_movsbq_m_r:
            __ MovSx(kB, kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_movswl_m_r:
            __ MovSx(kW, kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_movswq_m_r:
            __ MovSx(kW, kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_movslq_m_r:
            __ MovSx(kL, kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        /* add */
        case x64::MOP_addb_r_r:
            __ Add(kB, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_addw_r_r:
            __ Add(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_addl_r_r:
            __ Add(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_addq_r_r:
            __ Add(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_addb_i_r:
            __ Add(kB, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_addw_i_r:
            __ Add(kW, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_addl_i_r:
            __ Add(kL, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_addq_i_r:
            __ Add(kQ, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_addb_m_r:
            __ Add(kB, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_addw_m_r:
            __ Add(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_addl_m_r:
            __ Add(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_addq_m_r:
            __ Add(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_addb_r_m:
            __ Add(kB, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_addw_r_m:
            __ Add(kW, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_addl_r_m:
            __ Add(kL, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_addq_r_m:
            __ Add(kQ, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_addb_i_m:
            __ Add(kB, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_addw_i_m:
            __ Add(kW, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_addl_i_m:
            __ Add(kL, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_addq_i_m:
            __ Add(kQ, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        /* movabs */
        case x64::MOP_movabs_i_r:
            __ Movabs(TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_movabs_l_r:
            __ Movabs(TransferLabel(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        /* push */
        case x64::MOP_pushq_r:
            __ Push(kQ, TransferReg(opnd0));
            break;
        /* pop */
        case x64::MOP_popq_r:
            __ Pop(kQ, TransferReg(opnd0));
            break;
        /* lea */
        case x64::MOP_leaw_m_r:
            __ Lea(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_leal_m_r:
            __ Lea(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_leaq_m_r:
            __ Lea(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        /* sub , sbb */
        case x64::MOP_subb_r_r:
            __ Sub(kB, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_subw_r_r:
            __ Sub(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_subl_r_r:
            __ Sub(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_subq_r_r:
            __ Sub(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_subb_i_r:
            __ Sub(kB, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_subw_i_r:
            __ Sub(kW, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_subl_i_r:
            __ Sub(kL, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_subq_i_r:
            __ Sub(kQ, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_subb_m_r:
            __ Sub(kB, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_subw_m_r:
            __ Sub(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_subl_m_r:
            __ Sub(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_subq_m_r:
            __ Sub(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_subb_r_m:
            __ Sub(kB, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_subw_r_m:
            __ Sub(kW, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_subl_r_m:
            __ Sub(kL, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_subq_r_m:
            __ Sub(kQ, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_subb_i_m:
            __ Sub(kB, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_subw_i_m:
            __ Sub(kW, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_subl_i_m:
            __ Sub(kL, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_subq_i_m:
            __ Sub(kQ, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        /* and */
        case x64::MOP_andb_r_r:
            __ And(kB, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_andw_r_r:
            __ And(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_andl_r_r:
            __ And(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_andq_r_r:
            __ And(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_andb_i_r:
            __ And(kB, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_andw_i_r:
            __ And(kW, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_andl_i_r:
            __ And(kL, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_andq_i_r:
            __ And(kQ, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_andb_m_r:
            __ And(kB, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_andw_m_r:
            __ And(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_andl_m_r:
            __ And(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_andq_m_r:
            __ And(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_andb_r_m:
            __ And(kB, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_andw_r_m:
            __ And(kW, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_andl_r_m:
            __ And(kL, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_andq_r_m:
            __ And(kQ, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_andb_i_m:
            __ And(kB, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_andw_i_m:
            __ And(kW, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_andl_i_m:
            __ And(kL, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_andq_i_m:
            __ And(kQ, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        /* or */
        case x64::MOP_orb_r_r:
            __ Or(kB, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_orw_r_r:
            __ Or(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_orl_r_r:
            __ Or(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_orq_r_r:
            __ Or(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_orb_m_r:
            __ Or(kB, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_orw_m_r:
            __ Or(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_orl_m_r:
            __ Or(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_orq_m_r:
            __ Or(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_orb_i_r:
            __ Or(kB, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_orw_i_r:
            __ Or(kW, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_orl_i_r:
            __ Or(kL, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_orq_i_r:
            __ Or(kQ, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_orb_r_m:
            __ Or(kB, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_orw_r_m:
            __ Or(kW, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_orl_r_m:
            __ Or(kL, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_orq_r_m:
            __ Or(kQ, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_orb_i_m:
            __ Or(kB, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_orw_i_m:
            __ Or(kW, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_orl_i_m:
            __ Or(kL, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_orq_i_m:
            __ Or(kQ, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        /* xor */
        case x64::MOP_xorb_r_r:
            __ Xor(kB, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_xorw_r_r:
            __ Xor(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_xorl_r_r:
            __ Xor(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_xorq_r_r:
            __ Xor(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_xorb_i_r:
            __ Xor(kB, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_xorw_i_r:
            __ Xor(kW, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_xorl_i_r:
            __ Xor(kL, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_xorq_i_r:
            __ Xor(kQ, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_xorb_m_r:
            __ Xor(kB, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_xorw_m_r:
            __ Xor(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_xorl_m_r:
            __ Xor(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_xorq_m_r:
            __ Xor(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_xorb_r_m:
            __ Xor(kB, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_xorw_r_m:
            __ Xor(kW, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_xorl_r_m:
            __ Xor(kL, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_xorq_r_m:
            __ Xor(kQ, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_xorb_i_m:
            __ Xor(kB, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_xorw_i_m:
            __ Xor(kW, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_xorl_i_m:
            __ Xor(kL, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_xorq_i_m:
            __ Xor(kQ, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        /* not */
        case x64::MOP_notb_r:
            __ Not(kB, TransferReg(opnd0));
            break;
        case x64::MOP_notw_r:
            __ Not(kW, TransferReg(opnd0));
            break;
        case x64::MOP_notl_r:
            __ Not(kL, TransferReg(opnd0));
            break;
        case x64::MOP_notq_r:
            __ Not(kQ, TransferReg(opnd0));
            break;
        case x64::MOP_notb_m:
            __ Not(kB, TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_notw_m:
            __ Not(kW, TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_notl_m:
            __ Not(kL, TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_notq_m:
            __ Not(kQ, TransferMem(opnd0, funcUniqueId));
            break;
        /* neg */
        case x64::MOP_negb_r:
            __ Neg(kB, TransferReg(opnd0));
            break;
        case x64::MOP_negw_r:
            __ Neg(kW, TransferReg(opnd0));
            break;
        case x64::MOP_negl_r:
            __ Neg(kL, TransferReg(opnd0));
            break;
        case x64::MOP_negq_r:
            __ Neg(kQ, TransferReg(opnd0));
            break;
        case x64::MOP_negb_m:
            __ Neg(kB, TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_negw_m:
            __ Neg(kW, TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_negl_m:
            __ Neg(kL, TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_negq_m:
            __ Neg(kQ, TransferMem(opnd0, funcUniqueId));
            break;
        /* div, cwd, cdq, cqo */
        case x64::MOP_idivw_r:
            __ Idiv(kW, TransferReg(opnd0));
            break;
        case x64::MOP_idivl_r:
            __ Idiv(kL, TransferReg(opnd0));
            break;
        case x64::MOP_idivq_r:
            __ Idiv(kQ, TransferReg(opnd0));
            break;
        case x64::MOP_idivw_m:
            __ Idiv(kW, TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_idivl_m:
            __ Idiv(kL, TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_idivq_m:
            __ Idiv(kQ, TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_divw_r:
            __ Div(kW, TransferReg(opnd0));
            break;
        case x64::MOP_divl_r:
            __ Div(kL, TransferReg(opnd0));
            break;
        case x64::MOP_divq_r:
            __ Div(kQ, TransferReg(opnd0));
            break;
        case x64::MOP_divw_m:
            __ Div(kW, TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_divl_m:
            __ Div(kL, TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_divq_m:
            __ Div(kQ, TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_cwd:
            __ Cwd();
            break;
        case x64::MOP_cdq:
            __ Cdq();
            break;
        case x64::MOP_cqo:
            __ Cqo();
            break;
        /* shl */
        case x64::MOP_shlb_r_r:
            __ Shl(kB, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shlw_r_r:
            __ Shl(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shll_r_r:
            __ Shl(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shlq_r_r:
            __ Shl(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shlb_i_r:
            __ Shl(kB, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shlw_i_r:
            __ Shl(kW, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shll_i_r:
            __ Shl(kL, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shlq_i_r:
            __ Shl(kQ, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shlb_r_m:
            __ Shl(kB, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_shlw_r_m:
            __ Shl(kW, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_shll_r_m:
            __ Shl(kL, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_shlq_r_m:
            __ Shl(kQ, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_shlb_i_m:
            __ Shl(kB, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_shlw_i_m:
            __ Shl(kW, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_shll_i_m:
            __ Shl(kL, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_shlq_i_m:
            __ Shl(kQ, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        /* sar */
        case x64::MOP_sarb_r_r:
            __ Sar(kB, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_sarw_r_r:
            __ Sar(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_sarl_r_r:
            __ Sar(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_sarq_r_r:
            __ Sar(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_sarb_i_r:
            __ Sar(kB, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_sarw_i_r:
            __ Sar(kW, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_sarl_i_r:
            __ Sar(kL, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_sarq_i_r:
            __ Sar(kQ, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_sarb_r_m:
            __ Sar(kB, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_sarw_r_m:
            __ Sar(kW, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_sarl_r_m:
            __ Sar(kL, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_sarq_r_m:
            __ Sar(kQ, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_sarb_i_m:
            __ Sar(kB, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_sarw_i_m:
            __ Sar(kW, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_sarl_i_m:
            __ Sar(kL, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_sarq_i_m:
            __ Sar(kQ, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        /* shr */
        case x64::MOP_shrb_r_r:
            __ Shr(kB, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shrw_r_r:
            __ Shr(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shrl_r_r:
            __ Shr(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shrq_r_r:
            __ Shr(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shrb_i_r:
            __ Shr(kB, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shrw_i_r:
            __ Shr(kW, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shrl_i_r:
            __ Shr(kL, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shrq_i_r:
            __ Shr(kQ, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_shrb_r_m:
            __ Shr(kB, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_shrw_r_m:
            __ Shr(kW, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_shrl_r_m:
            __ Shr(kL, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_shrq_r_m:
            __ Shr(kQ, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_shrb_i_m:
            __ Shr(kB, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_shrw_i_m:
            __ Shr(kW, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_shrl_i_m:
            __ Shr(kL, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_shrq_i_m:
            __ Shr(kQ, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        /* jmp */
        case x64::MOP_jmpq_r:
            __ Jmp(TransferReg(opnd0));
            break;
        case x64::MOP_jmpq_m:
            __ Jmp(TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_jmpq_l:
            __ Jmp(TransferLabel(opnd0, funcUniqueId));
            break;
        /* je, jne */
        case x64::MOP_je_l:
            __ Je(TransferLabel(opnd0, funcUniqueId));
            break;
        case x64::MOP_ja_l:
            __ Ja(TransferLabel(opnd0, funcUniqueId));
            break;
        case x64::MOP_jae_l:
            __ Jae(TransferLabel(opnd0, funcUniqueId));
            break;
        case x64::MOP_jne_l:
            __ Jne(TransferLabel(opnd0, funcUniqueId));
            break;
        case x64::MOP_jb_l:
            __ Jb(TransferLabel(opnd0, funcUniqueId));
            break;
        case x64::MOP_jbe_l:
            __ Jbe(TransferLabel(opnd0, funcUniqueId));
            break;
        case x64::MOP_jg_l:
            __ Jg(TransferLabel(opnd0, funcUniqueId));
            break;
        case x64::MOP_jge_l:
            __ Jge(TransferLabel(opnd0, funcUniqueId));
            break;
        case x64::MOP_jl_l:
            __ Jl(TransferLabel(opnd0, funcUniqueId));
            break;
        case x64::MOP_jle_l:
            __ Jle(TransferLabel(opnd0, funcUniqueId));
            break;
        /* cmp */
        case x64::MOP_cmpb_r_r:
            __ Cmp(kB, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmpw_r_r:
            __ Cmp(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmpl_r_r:
            __ Cmp(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmpq_r_r:
            __ Cmp(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmpb_i_r:
            __ Cmp(kB, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmpw_i_r:
            __ Cmp(kW, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmpl_i_r:
            __ Cmp(kL, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmpq_i_r:
            __ Cmp(kQ, TransferImm(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmpb_m_r:
            __ Cmp(kB, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmpw_m_r:
            __ Cmp(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmpl_m_r:
            __ Cmp(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmpq_m_r:
            __ Cmp(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmpb_r_m:
            __ Cmp(kB, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_cmpw_r_m:
            __ Cmp(kW, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_cmpl_r_m:
            __ Cmp(kL, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_cmpq_r_m:
            __ Cmp(kQ, TransferReg(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_cmpb_i_m:
            __ Cmp(kB, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_cmpw_i_m:
            __ Cmp(kW, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_cmpl_i_m:
            __ Cmp(kL, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_cmpq_i_m:
            __ Cmp(kQ, TransferImm(opnd0), TransferMem(opnd1, funcUniqueId));
            break;
        case x64::MOP_testq_r_r:
            __ Test(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        /* setcc */
        case x64::MOP_seta_r:
            __ Seta(TransferReg(opnd0));
            break;
        case x64::MOP_setae_r:
            __ Setae(TransferReg(opnd0));
            break;
        case x64::MOP_setb_r:
            __ Setb(TransferReg(opnd0));
            break;
        case x64::MOP_seto_r:
            __ Seto(TransferReg(opnd0));
            break;
        case x64::MOP_setbe_r:
            __ Setbe(TransferReg(opnd0));
            break;
        case x64::MOP_sete_r:
            __ Sete(TransferReg(opnd0));
            break;
        case x64::MOP_setg_r:
            __ Setg(TransferReg(opnd0));
            break;
        case x64::MOP_setge_r:
            __ Setge(TransferReg(opnd0));
            break;
        case x64::MOP_setl_r:
            __ Setl(TransferReg(opnd0));
            break;
        case x64::MOP_setle_r:
            __ Setle(TransferReg(opnd0));
            break;
        case x64::MOP_setne_r:
            __ Setne(TransferReg(opnd0));
            break;
        case x64::MOP_seta_m:
            __ Seta(TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_setae_m:
            __ Setae(TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_setb_m:
            __ Setb(TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_seto_m:
            __ Seto(TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_setbe_m:
            __ Setbe(TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_sete_m:
            __ Sete(TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_setl_m:
            __ Setl(TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_setle_m:
            __ Setle(TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_setg_m:
            __ Setg(TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_setge_m:
            __ Setge(TransferMem(opnd0, funcUniqueId));
            break;
        case x64::MOP_setne_m:
            __ Setne(TransferMem(opnd0, funcUniqueId));
            break;
        /* cmova & cmovae */
        case x64::MOP_cmovaw_r_r:
            __ Cmova(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmoval_r_r:
            __ Cmova(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovaq_r_r:
            __ Cmova(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovaw_m_r:
            __ Cmova(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmoval_m_r:
            __ Cmova(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovaq_m_r:
            __ Cmova(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovaew_r_r:
            __ Cmovae(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovael_r_r:
            __ Cmovae(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovaeq_r_r:
            __ Cmovae(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovaew_m_r:
            __ Cmovae(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovael_m_r:
            __ Cmovae(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovaeq_m_r:
            __ Cmovae(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        /* cmovb & cmovbe */
        case x64::MOP_cmovbw_r_r:
            __ Cmovb(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovbl_r_r:
            __ Cmovb(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovbq_r_r:
            __ Cmovb(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovbw_m_r:
            __ Cmovb(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovbl_m_r:
            __ Cmovb(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovbq_m_r:
            __ Cmovb(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovbew_r_r:
            __ Cmovbe(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovbel_r_r:
            __ Cmovbe(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovbeq_r_r:
            __ Cmovbe(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovbew_m_r:
            __ Cmovbe(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovbel_m_r:
            __ Cmovbe(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovbeq_m_r:
            __ Cmovbe(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        /* cmove */
        case x64::MOP_cmovew_r_r:
            __ Cmove(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovel_r_r:
            __ Cmove(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmoveq_r_r:
            __ Cmove(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovew_m_r:
            __ Cmove(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovel_m_r:
            __ Cmove(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmoveq_m_r:
            __ Cmove(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        /* cmovg & cmovge */
        case x64::MOP_cmovgw_r_r:
            __ Cmovg(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovgl_r_r:
            __ Cmovg(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovgq_r_r:
            __ Cmovg(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovgw_m_r:
            __ Cmovg(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovgl_m_r:
            __ Cmovg(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovgq_m_r:
            __ Cmovg(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovgew_r_r:
            __ Cmovge(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovgel_r_r:
            __ Cmovge(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovgeq_r_r:
            __ Cmovge(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovgew_m_r:
            __ Cmovge(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovgel_m_r:
            __ Cmovge(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovgeq_m_r:
            __ Cmovge(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        /* cmovl & cmovle */
        case x64::MOP_cmovlw_r_r:
            __ Cmovl(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovll_r_r:
            __ Cmovl(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovlq_r_r:
            __ Cmovl(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovlw_m_r:
            __ Cmovl(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovll_m_r:
            __ Cmovl(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovlq_m_r:
            __ Cmovl(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovlew_r_r:
            __ Cmovle(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovlel_r_r:
            __ Cmovle(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovleq_r_r:
            __ Cmovle(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovlew_m_r:
            __ Cmovle(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovlel_m_r:
            __ Cmovle(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovleq_m_r:
            __ Cmovle(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        /* cmovne */
        case x64::MOP_cmovnew_r_r:
            __ Cmovne(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovnel_r_r:
            __ Cmovne(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovneq_r_r:
            __ Cmovne(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovnew_m_r:
            __ Cmovne(kW, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovnel_m_r:
            __ Cmovne(kL, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovneq_m_r:
            __ Cmovne(kQ, TransferMem(opnd0, funcUniqueId), TransferReg(opnd1));
            break;
        case x64::MOP_cmovow_r_r:
            __ Cmovo(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovol_r_r:
            __ Cmovo(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_cmovoq_r_r:
            __ Cmovo(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        /* call */
        case x64::MOP_callq_r: {
            __ Call(kQ, TransferReg(opnd0));
            if (insn.GetStackMap() != nullptr) {
                auto referenceMap = insn.GetStackMap()->GetReferenceMap().SerializeInfo();
                auto deoptInfo = insn.GetStackMap()->GetDeoptInfo().SerializeInfo();
                __ RecordStackmap(referenceMap, deoptInfo);
            }
            break;
        }
        case x64::MOP_callq_l: {
            __ Call(kQ, TransferFuncName(opnd0));
            if (insn.GetStackMap() != nullptr) {
                auto referenceMap = insn.GetStackMap()->GetReferenceMap().SerializeInfo();
                auto deoptInfo = insn.GetStackMap()->GetDeoptInfo().SerializeInfo();
                __ RecordStackmap(referenceMap, deoptInfo);
            }
            break;
        }

        case x64::MOP_callq_m: {
            __ Call(kQ, TransferMem(opnd0, funcUniqueId));
            if (insn.GetStackMap() != nullptr) {
                auto referenceMap = insn.GetStackMap()->GetReferenceMap().SerializeInfo();
                auto deoptInfo = insn.GetStackMap()->GetDeoptInfo().SerializeInfo();
                __ RecordStackmap(referenceMap, deoptInfo);
            }
            break;
        }

        /* ret */
        case x64::MOP_retq:
            __ Ret();
            break;
        case x64::MOP_leaveq:
            __ Leave();
            break;
        /* imul */
        case x64::MOP_imulw_r_r:
            __ Imul(kW, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_imull_r_r:
            __ Imul(kL, TransferReg(opnd0), TransferReg(opnd1));
            break;
        case x64::MOP_imulq_r_r:
            __ Imul(kQ, TransferReg(opnd0), TransferReg(opnd1));
            break;
        /* nop */
        case x64::MOP_nop:
            __ Nop();
            break;
        /* byte swap */
        case x64::MOP_bswapl_r:
            __ Bswap(kL, TransferReg(opnd0));
            break;
        case x64::MOP_bswapq_r:
            __ Bswap(kQ, TransferReg(opnd0));
            break;
        case x64::MOP_xchgb_r_r:
            __ Xchg(kB, TransferReg(opnd0), TransferReg(opnd1));
            break;
        /* pseudo instruction */
        case x64::MOP_pseudo_ret_int:
            __ DealWithPseudoInst(curMd.GetName());
            break;
        default: {
            insn.Dump();
            LogInfo::MapleLogger() << "\n";
            FATAL(kLncFatal, "unsupported instruction");
            break;
        }
    }
}

void X64Emitter::EmitFunctionHeader(CGFunc &cgFunc)
{
    const MIRSymbol *funcSymbol = cgFunc.GetFunction().GetFuncSymbol();
    uint32 symIdx = funcSymbol->GetNameStrIdx().get();
    const string &symName = funcSymbol->GetName();
    __ StoreNameIntoSymMap(symIdx, symName);

    SymbolAttr funcAttr = kSAGlobal;
    if (funcSymbol->GetFunction()->GetAttr(FUNCATTR_weak)) {
        funcAttr = kSAWeak;
    } else if (funcSymbol->GetFunction()->GetAttr(FUNCATTR_local)) {
        funcAttr = kSALocal;
    } else if (!cgFunc.GetCG()->GetMIRModule()->IsCModule()) {
        funcAttr = kSAHidden;
    }
    if (cgFunc.GetFunction().GetAttr(FUNCATTR_section)) {
        const string &sectionName = cgFunc.GetFunction().GetAttrs().GetPrefixSectionName();
        __ EmitFunctionHeader(symIdx, funcAttr, &sectionName);
    } else {
        __ EmitFunctionHeader(symIdx, funcAttr, nullptr);
    }
}

void X64Emitter::EmitBBHeaderLabel(CGFunc &cgFunc, LabelIdx labIdx, uint32 freq)
{
    uint32 funcUniqueId = cgFunc.GetUniqueID();
    /* Concatenate BB Label Name and its idx */
    string bbLabel = ".L.";
    bbLabel.append(to_string(funcUniqueId));
    bbLabel.append("__");
    bbLabel.append(to_string(labIdx));
    int64 labelSymIdx = CalculateLabelSymIdx(funcUniqueId, static_cast<int64>(labIdx));
    __ StoreNameIntoSymMap(labelSymIdx, bbLabel);

    if (cgFunc.GetCG()->GenerateVerboseCG()) {
        const string &labelName = cgFunc.GetFunction().GetLabelTab()->GetName(labIdx);
        /* If label name has @ as its first char, it is not from MIR */
        if (!labelName.empty() && labelName.at(0) != '@') {
            __ EmitBBLabel(labelSymIdx, true, freq, &labelName);
        } else {
            __ EmitBBLabel(labelSymIdx, true, freq);
        }
    } else {
        __ EmitBBLabel(labelSymIdx);
    }
}

/* Specially, emit switch table here */
void X64Emitter::EmitJmpTable(const CGFunc &cgFunc)
{
    for (auto &it : cgFunc.GetEmitStVec()) {
        MIRSymbol *st = it.second;
        DEBUG_ASSERT(st->IsReadOnly(), "NYI");
        uint32 symIdx = st->GetNameStrIdx().get();
        const string &symName = st->GetName();
        __ StoreNameIntoSymMap(symIdx, symName);

        MIRAggConst *arrayConst = safe_cast<MIRAggConst>(st->GetKonst());
        CHECK_NULL_FATAL(arrayConst);
        uint32 funcUniqueId = cgFunc.GetUniqueID();
        vector<int64> labelSymIdxs;
        for (size_t i = 0; i < arrayConst->GetConstVec().size(); i++) {
            MIRLblConst *lblConst = safe_cast<MIRLblConst>(arrayConst->GetConstVecItem(i));
            CHECK_NULL_FATAL(lblConst);
            uint32 labelIdx = lblConst->GetValue();
            string labelName = ".L." + to_string(funcUniqueId) + "__" + to_string(labelIdx);
            int64 labelSymIdx = CalculateLabelSymIdx(funcUniqueId, labelIdx);
            __ StoreNameIntoSymMap(labelSymIdx, labelName);
            labelSymIdxs.push_back(labelSymIdx);
        }
        __ EmitJmpTableElem(symIdx, labelSymIdxs);
    }
}

void X64Emitter::EmitFunctionFoot(CGFunc &cgFunc)
{
    const MIRSymbol *funcSymbol = cgFunc.GetFunction().GetFuncSymbol();
    uint32 symIdx = funcSymbol->GetNameStrIdx().get();
    SymbolAttr funcAttr = kSALocal;
    if (funcSymbol->GetFunction()->GetAttr(FUNCATTR_weak)) {
        funcAttr = kSAWeak;
    } else if (funcSymbol->GetFunction()->GetAttr(FUNCATTR_local)) {
        funcAttr = kSALocal;
    } else if (!funcSymbol->GetFunction()->GetAttr(FUNCATTR_static)) {
        funcAttr = kSAGlobal;
    }
    __ EmitFunctionFoot(symIdx, funcAttr);
}

uint64 X64Emitter::EmitStructure(MIRConst &mirConst, CG &cg, bool belongsToDataSec)
{
    uint32 subStructFieldCounts = 0;
    uint64 valueSize = EmitStructure(mirConst, cg, subStructFieldCounts, belongsToDataSec);
    return valueSize;
}

uint64 X64Emitter::EmitStructure(MIRConst &mirConst, CG &cg, uint32 &subStructFieldCounts, bool belongsToDataSec)
{
    StructEmitInfo *sEmitInfo = cg.GetMIRModule()->GetMemPool()->New<StructEmitInfo>();
    CHECK_NULL_FATAL(sEmitInfo);
    MIRType &mirType = mirConst.GetType();
    MIRAggConst &structCt = static_cast<MIRAggConst &>(mirConst);
    MIRStructType &structType = static_cast<MIRStructType &>(mirType);
    uint8 structPack = static_cast<uint8>(structType.GetTypeAttrs().GetPack());
    uint64 valueSize = 0;
    MIRTypeKind structKind = structType.GetKind();
    /* all elements of struct. */
    uint8 num = structKind == kTypeUnion ? 1 : static_cast<uint8>(structType.GetFieldsSize());
    BECommon *beCommon = Globals::GetInstance()->GetBECommon();
    /* total size of emitted elements size. */
    uint64 sizeInByte = GetSymbolSize(structType.GetTypeIndex());
    uint32 fieldIdx = structKind == kTypeUnion ? structCt.GetFieldIdItem(0) : 1;
    for (uint32 i = 0; i < num; ++i) {
        MIRConst *elemConst =
            structKind == kTypeStruct ? structCt.GetAggConstElement(i + 1) : structCt.GetAggConstElement(fieldIdx);
        MIRType *elemType = structKind == kTypeUnion ? &(elemConst->GetType()) : structType.GetElemType(i);
        MIRType *nextElemType = i != static_cast<uint32>(num - 1) ? structType.GetElemType(i + 1) : nullptr;
        uint64 elemSize = GetSymbolSize(elemType->GetTypeIndex());
        uint8 charBitWidth = GetPrimTypeSize(PTY_i8) * k8Bits;
        MIRTypeKind elemKind = elemType->GetKind();
        if (elemKind == kTypeBitField) {
            if (elemConst == nullptr) {
                MIRIntConst *zeroFill = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *elemType);
                elemConst = zeroFill;
            }
            pair<int32, int32> fieldOffsetPair = beCommon->GetFieldOffset(structType, fieldIdx);
            uint64 fieldOffset =
                static_cast<uint64>(static_cast<int64>(fieldOffsetPair.first)) * static_cast<uint64>(charBitWidth) +
                static_cast<uint64>(static_cast<int64>(fieldOffsetPair.second));
            EmitBitField(*sEmitInfo, *elemConst, nextElemType, fieldOffset);
        } else {
            if (elemConst != nullptr) {
                if (IsPrimitiveVector(elemType->GetPrimType())) {
                    valueSize += EmitVector(*elemConst);
                } else if (IsPrimitiveScalar(elemType->GetPrimType())) {
                    valueSize += EmitSingleElement(*elemConst, belongsToDataSec, true);
                } else if (elemKind == kTypeArray) {
                    if (elemType->GetSize() != 0) {
                        valueSize += EmitArray(*elemConst, cg, belongsToDataSec);
                    }
                } else if (elemKind == kTypeStruct || elemKind == kTypeClass || elemKind == kTypeUnion) {
                    valueSize += EmitStructure(*elemConst, cg, subStructFieldCounts, belongsToDataSec);
                    fieldIdx += subStructFieldCounts;
                } else {
                    DEBUG_ASSERT(false, "should not run here");
                }
            } else {
                __ EmitNull(elemSize);
            }
            sEmitInfo->IncreaseTotalSize(elemSize);
            sEmitInfo->SetNextFieldOffset(sEmitInfo->GetTotalSize() * charBitWidth);
        }

        if (nextElemType != nullptr && nextElemType->GetKind() != kTypeBitField) {
            DEBUG_ASSERT(i < static_cast<uint32>(num - 1), "NYI");
            uint8 nextAlign = Globals::GetInstance()->GetBECommon()->GetTypeAlign(nextElemType->GetTypeIndex());
            auto fieldAttr = structType.GetFields()[i + 1].second.second;
            nextAlign = fieldAttr.IsPacked() ? 1 : min(nextAlign, structPack);
            DEBUG_ASSERT(nextAlign != 0, "expect non-zero");
            /* append size, append 0 when align need. */
            uint64 totalSize = sEmitInfo->GetTotalSize();
            uint64 psize = (totalSize % nextAlign == 0) ? 0 : (nextAlign - (totalSize % nextAlign));
            /* element is uninitialized, emit null constant. */
            if (psize != 0) {
                __ EmitNull(psize);
                sEmitInfo->IncreaseTotalSize(psize);
                sEmitInfo->SetNextFieldOffset(sEmitInfo->GetTotalSize() * charBitWidth);
            }
        }
        fieldIdx++;
    }
    if (structType.GetKind() == kTypeStruct) {
        /* The reason of subtracting one is that fieldIdx adds one at the end of the cycle. */
        subStructFieldCounts = fieldIdx - 1;
    } else if (structType.GetKind() == kTypeUnion) {
        subStructFieldCounts = static_cast<uint32>(beCommon->GetStructFieldCount(structType.GetTypeIndex()));
    }

    uint64 opSize = sizeInByte - sEmitInfo->GetTotalSize();
    if (opSize != 0) {
        __ EmitNull(opSize);
    }
    return valueSize;
}

uint64 X64Emitter::EmitVector(MIRConst &mirConst, bool belongsToDataSec)
{
    MIRType &mirType = mirConst.GetType();
    MIRAggConst &vecCt = static_cast<MIRAggConst &>(mirConst);
    size_t uNum = vecCt.GetConstVec().size();
    uint64 valueSize = 0;
    for (size_t i = 0; i < uNum; ++i) {
        MIRConst *elemConst = vecCt.GetConstVecItem(i);
        if (IsPrimitiveScalar(elemConst->GetType().GetPrimType())) {
            uint64 elemSize = EmitSingleElement(*elemConst, belongsToDataSec);
            valueSize += elemSize;
        } else {
            DEBUG_ASSERT(false, "EmitVector: should not run here");
        }
    }
    size_t lanes = GetVecLanes(mirType.GetPrimType());
    if (lanes > uNum) {
        MIRIntConst zConst(0, vecCt.GetConstVecItem(0)->GetType());
        for (size_t i = uNum; i < lanes; i++) {
            uint64 elemSize = EmitSingleElement(zConst, belongsToDataSec);
            valueSize += elemSize;
        }
    }
    return valueSize;
}

uint64 X64Emitter::EmitArray(MIRConst &mirConst, CG &cg, bool belongsToDataSec)
{
    MIRType &mirType = mirConst.GetType();
    MIRAggConst &arrayCt = static_cast<MIRAggConst &>(mirConst);
    MIRArrayType &arrayType = static_cast<MIRArrayType &>(mirType);
    size_t uNum = arrayCt.GetConstVec().size();
    uint32 dim = arrayType.GetSizeArrayItem(0);
    TyIdx elmTyIdx = arrayType.GetElemTyIdx();
    MIRType *subTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(elmTyIdx);
    uint64 valueSize = 0;
    if (uNum == 0 && dim) {
        while (subTy->GetKind() == kTypeArray) {
            MIRArrayType *aSubTy = static_cast<MIRArrayType *>(subTy);
            if (aSubTy->GetSizeArrayItem(0) > 0) {
                dim *= (aSubTy->GetSizeArrayItem(0));
            }
            elmTyIdx = aSubTy->GetElemTyIdx();
            subTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(elmTyIdx);
        }
    }
    for (size_t i = 0; i < uNum; ++i) {
        MIRConst *elemConst = arrayCt.GetConstVecItem(i);
        if (IsPrimitiveVector(subTy->GetPrimType())) {
            valueSize += EmitVector(*elemConst, belongsToDataSec);
        } else if (IsPrimitiveScalar(elemConst->GetType().GetPrimType())) {
            if (cg.GetMIRModule()->IsCModule()) {
                bool strLiteral = false;
                if (arrayType.GetDim() == 1) {
                    MIRType *ety = arrayType.GetElemType();
                    if (ety->GetPrimType() == PTY_i8 || ety->GetPrimType() == PTY_u8) {
                        strLiteral = true;
                    }
                }
                valueSize += EmitSingleElement(*elemConst, belongsToDataSec, !strLiteral);
            } else {
                valueSize += EmitSingleElement(*elemConst, belongsToDataSec);
            }
        } else if (elemConst->GetType().GetKind() == kTypeArray) {
            valueSize += EmitArray(*elemConst, cg, belongsToDataSec);
        } else if (elemConst->GetType().GetKind() == kTypeStruct || elemConst->GetType().GetKind() == kTypeClass ||
                   elemConst->GetType().GetKind() == kTypeUnion) {
            valueSize += EmitStructure(*elemConst, cg);
        } else if (elemConst->GetKind() == kConstAddrofFunc) {
            valueSize += EmitSingleElement(*elemConst, belongsToDataSec);
        } else {
            DEBUG_ASSERT(false, "should not run here");
        }
    }
    int64 iNum = (arrayType.GetSizeArrayItem(0) > 0) ? (static_cast<int64>(arrayType.GetSizeArrayItem(0))) - uNum : 0;
    if (iNum > 0) {
        if (uNum > 0) {
            uint64 unInSizeInByte =
                static_cast<uint64>(iNum) *
                static_cast<uint64>(GetSymbolSize(arrayCt.GetConstVecItem(0)->GetType().GetTypeIndex()));
            if (unInSizeInByte != 0) {
                __ EmitNull(unInSizeInByte);
            }
        } else {
            uint64 sizeInByte = GetSymbolSize(elmTyIdx) * dim;
            __ EmitNull(sizeInByte);
        }
    }
    return valueSize;
}

void X64Emitter::EmitAddrofElement(MIRConst &mirConst, bool belongsToDataSec)
{
    MIRAddrofConst &symAddr = static_cast<MIRAddrofConst &>(mirConst);
    StIdx stIdx = symAddr.GetSymbolIndex();
    MIRSymbol *symAddrSym =
        stIdx.IsGlobal()
            ? GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx())
            : CG::GetCurCGFunc()->GetMirModule().CurFunction()->GetSymTab()->GetSymbolFromStIdx(stIdx.Idx());
    string addrName = symAddrSym->GetName();
    if (!stIdx.IsGlobal() && symAddrSym->GetStorageClass() == kScPstatic) {
        uint32 funcUniqueId = CG::GetCurCGFunc()->GetUniqueID();
        addrName += to_string(funcUniqueId);
    }
    uint32 symIdx = symAddrSym->GetNameStrIdx();
    int32 symAddrOfs = 0;
    int32 structFieldOfs = 0;
    if (symAddr.GetOffset() != 0) {
        symAddrOfs = symAddr.GetOffset();
    }
    if (symAddr.GetFieldID() > 1) {
        MIRStructType *structType = static_cast<MIRStructType *>(symAddrSym->GetType());
        DEBUG_ASSERT(structType != nullptr, "EmitScalarConstant: non-zero fieldID for non-structure");
        structFieldOfs = Globals::GetInstance()->GetBECommon()->GetFieldOffset(*structType, symAddr.GetFieldID()).first;
    }
    __ StoreNameIntoSymMap(symIdx, addrName);
    __ EmitAddrValue(symIdx, symAddrOfs, structFieldOfs, belongsToDataSec);
}

uint32 X64Emitter::EmitSingleElement(MIRConst &mirConst, bool belongsToDataSec, bool isIndirect)
{
    MIRType &elmType = mirConst.GetType();
    uint64 elemSize = elmType.GetSize();
    MIRConstKind kind = mirConst.GetKind();
    switch (kind) {
        case kConstAddrof:
            EmitAddrofElement(mirConst, belongsToDataSec);
            break;
        case kConstAddrofFunc: {
            MIRAddroffuncConst &funcAddr = static_cast<MIRAddroffuncConst &>(mirConst);
            MIRFunction *func = GlobalTables::GetFunctionTable().GetFuncTable().at(funcAddr.GetValue());
            MIRSymbol *symAddrSym = GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx());

            uint32 symIdx = symAddrSym->GetNameStrIdx();
            const string &name = symAddrSym->GetName();
            __ StoreNameIntoSymMap(symIdx, name);
            __ EmitAddrOfFuncValue(symIdx, belongsToDataSec);
            break;
        }
        case kConstInt: {
            MIRIntConst &intCt = static_cast<MIRIntConst &>(mirConst);
            uint32 sizeInBits = elemSize << kLeftShift3Bits;
            if (intCt.GetActualBitWidth() > sizeInBits) {
                DEBUG_ASSERT(false, "actual value is larger than expected");
            }
            int64 value = intCt.GetExtValue();
            __ EmitIntValue(value, elemSize, belongsToDataSec);
            break;
        }
        case kConstLblConst: {
            MIRLblConst &lbl = static_cast<MIRLblConst &>(mirConst);
            uint32 labelIdx = lbl.GetValue();
            uint32 funcUniqueId = lbl.GetPUIdx();
            string labelName = ".L." + to_string(funcUniqueId) + "__" + to_string(labelIdx);
            int64 symIdx = CalculateLabelSymIdx(funcUniqueId, labelIdx);
            __ StoreNameIntoSymMap(symIdx, labelName);
            __ EmitLabelValue(symIdx, belongsToDataSec);
            break;
        }
        case kConstStrConst: {
            MIRStrConst &strCt = static_cast<MIRStrConst &>(mirConst);
            if (isIndirect) {
                uint32 strIdx = strCt.GetValue().GetIdx();
                string strName = ".LSTR__" + to_string(strIdx);
                int64 strSymIdx = CalculateStrLabelSymIdx(GlobalTables::GetGsymTable().GetSymbolTableSize(), strIdx);
                stringPtr.push_back(strIdx);
                __ StoreNameIntoSymMap(strSymIdx, strName);
                __ EmitIndirectString(strSymIdx, belongsToDataSec);
            } else {
                const string &ustr = GlobalTables::GetUStrTable().GetStringFromStrIdx(strCt.GetValue());
                __ EmitDirectString(ustr, belongsToDataSec);
            }
            break;
        }
        default:
            FATAL(kLncFatal, "EmitSingleElement: unsupport variable kind");
            break;
    }
    return elemSize;
}

void X64Emitter::EmitBitField(StructEmitInfo &structEmitInfo, MIRConst &mirConst, const MIRType *nextType,
                              uint64 fieldOffset, bool belongsToDataSec)
{
    MIRType &mirType = mirConst.GetType();
    if (fieldOffset > structEmitInfo.GetNextFieldOffset()) {
        uint16 curFieldOffset = structEmitInfo.GetNextFieldOffset() - structEmitInfo.GetCombineBitFieldWidth();
        structEmitInfo.SetCombineBitFieldWidth(fieldOffset - curFieldOffset);
        EmitCombineBfldValue(structEmitInfo);
        DEBUG_ASSERT(structEmitInfo.GetNextFieldOffset() <= fieldOffset,
                     "structEmitInfo's nextFieldOffset > fieldOffset");
        structEmitInfo.SetNextFieldOffset(fieldOffset);
    }
    uint32 fieldSize = static_cast<MIRBitFieldType &>(mirType).GetFieldSize();
    MIRIntConst &fieldValue = static_cast<MIRIntConst &>(mirConst);
    /* Truncate the size of FieldValue to the bit field size. */
    if (fieldSize < fieldValue.GetActualBitWidth()) {
        fieldValue.Trunc(fieldSize);
    }
    /* Clear higher Bits for signed value  */
    if (structEmitInfo.GetCombineBitFieldValue() != 0) {
        structEmitInfo.SetCombineBitFieldValue((~(~0ULL << structEmitInfo.GetCombineBitFieldWidth())) &
                                               structEmitInfo.GetCombineBitFieldValue());
    }
    if (CGOptions::IsBigEndian()) {
        uint64 beValue = static_cast<uint64>(fieldValue.GetExtValue());
        if (fieldValue.IsNegative()) {
            beValue = beValue - ((beValue >> fieldSize) << fieldSize);
        }
        structEmitInfo.SetCombineBitFieldValue((structEmitInfo.GetCombineBitFieldValue() << fieldSize) + beValue);
    } else {
        structEmitInfo.SetCombineBitFieldValue(
            (static_cast<uint64>(fieldValue.GetExtValue()) << structEmitInfo.GetCombineBitFieldWidth()) +
            structEmitInfo.GetCombineBitFieldValue());
    }
    structEmitInfo.IncreaseCombineBitFieldWidth(fieldSize);
    structEmitInfo.IncreaseNextFieldOffset(fieldSize);
    if ((nextType == nullptr) || (nextType->GetKind() != kTypeBitField)) {
        /* emit structEmitInfo->combineBitFieldValue */
        EmitCombineBfldValue(structEmitInfo);
    }
}

void X64Emitter::EmitCombineBfldValue(StructEmitInfo &structEmitInfo, bool belongsToDataSec)
{
    uint8 charBitWidth = GetPrimTypeSize(PTY_i8) * k8Bits;
    const uint64 kGetLow8Bits = 0x00000000000000ffUL;
    auto emitBfldValue = [&structEmitInfo, charBitWidth, belongsToDataSec, this](bool flag) {
        while (structEmitInfo.GetCombineBitFieldWidth() > charBitWidth) {
            uint8 shift = flag ? (structEmitInfo.GetCombineBitFieldWidth() - charBitWidth) : 0U;
            uint64 tmp = (structEmitInfo.GetCombineBitFieldValue() >> shift) & kGetLow8Bits;
            __ EmitBitFieldValue(tmp, belongsToDataSec);
            structEmitInfo.DecreaseCombineBitFieldWidth(charBitWidth);
            uint64 value =
                flag ? structEmitInfo.GetCombineBitFieldValue() - (tmp << structEmitInfo.GetCombineBitFieldWidth())
                     : structEmitInfo.GetCombineBitFieldValue() >> charBitWidth;
            structEmitInfo.SetCombineBitFieldValue(value);
        }
    };
    if (CGOptions::IsBigEndian()) {
        /*
         * If the total number of bits in the bit field is not a multiple of 8,
         * the bits must be aligned to 8 bits to prevent errors in the emit.
         */
        auto width = static_cast<uint8>(RoundUp(structEmitInfo.GetCombineBitFieldWidth(), charBitWidth));
        if (structEmitInfo.GetCombineBitFieldWidth() < width) {
            structEmitInfo.SetCombineBitFieldValue(structEmitInfo.GetCombineBitFieldValue()
                                                   << (width - structEmitInfo.GetCombineBitFieldWidth()));
            structEmitInfo.IncreaseCombineBitFieldWidth(
                static_cast<uint8>(width - structEmitInfo.GetCombineBitFieldWidth()));
        }
        emitBfldValue(true);
    } else {
        emitBfldValue(false);
    }
    if (structEmitInfo.GetCombineBitFieldWidth() != 0) {
        uint64 value = structEmitInfo.GetCombineBitFieldValue() & kGetLow8Bits;
        __ EmitBitFieldValue(value, belongsToDataSec);
    }
    CHECK_FATAL(charBitWidth != 0, "divide by zero");
    if ((structEmitInfo.GetNextFieldOffset() % charBitWidth) != 0) {
        uint8 value = charBitWidth - static_cast<uint8>((structEmitInfo.GetNextFieldOffset() % charBitWidth));
        structEmitInfo.IncreaseNextFieldOffset(value);
    }
    structEmitInfo.SetTotalSize(structEmitInfo.GetNextFieldOffset() / charBitWidth);
    structEmitInfo.SetCombineBitFieldValue(0);
    structEmitInfo.SetCombineBitFieldWidth(0);
}

void X64Emitter::EmitLocalVariable(CGFunc &cgFunc)
{
    if (!cgFunc.GetCG()->GetMIRModule()->IsCModule()) {
        return;
    }
    /* function local pstatic initialization */
    MIRSymbolTable *lSymTab = cgFunc.GetFunction().GetSymTab();
    if (lSymTab != nullptr) {
        uint32 funcUniqueId = cgFunc.GetUniqueID();
        size_t lsize = lSymTab->GetSymbolTableSize();
        vector<string> emittedLocalSym;
        for (uint32 i = 0; i < lsize; i++) {
            MIRSymbol *symbol = lSymTab->GetSymbolFromStIdx(i);
            if (symbol != nullptr && symbol->GetStorageClass() == kScPstatic) {
                const string &symbolName = symbol->GetName() + to_string(funcUniqueId);
                /* Local static names can repeat, if repeat, pass */
                bool found = false;
                for (auto name : emittedLocalSym) {
                    if (name == symbolName) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    continue;
                }
                emittedLocalSym.push_back(symbolName);

                uint32 symIdx = symbol->GetNameStrIdx().get();
                __ StoreNameIntoSymMap(symIdx, symbolName, true);

                MIRConst *ct = symbol->GetKonst();
                MIRType *ty = symbol->GetType();
                uint64 sizeInByte = GetSymbolSize(ty->GetTypeIndex());
                uint8 alignInByte = GetSymbolAlign(*symbol);
                if (ct == nullptr) {
                    alignInByte = GetSymbolAlign(*symbol, true);
                    __ EmitVariable(symIdx, sizeInByte, alignInByte, kSALocal, kSBss);
                } else {
                    MIRTypeKind kind = ty->GetKind();
                    uint64 valueSize = 0;
                    __ EmitVariable(symIdx, sizeInByte, alignInByte, kSALocal, kSData);
                    if (kind == kTypeStruct || kind == kTypeUnion || kind == kTypeClass) {
                        valueSize = EmitStructure(*ct, *cgFunc.GetCG());
                    } else if (IsPrimitiveVector(ty->GetPrimType())) {
                        valueSize = EmitVector(*ct);
                    } else if (kind == kTypeArray) {
                        valueSize = EmitArray(*ct, *cgFunc.GetCG());
                    } else {
                        valueSize = EmitSingleElement(*ct, true);
                    }
                    __ PostEmitVariable(symIdx, kSALocal, valueSize);
                }
            }
        }
    }
}

void X64Emitter::EmitStringPointers()
{
    for (uint32 strIdx : stringPtr) {
        string ustr = GlobalTables::GetUStrTable().GetStringFromStrIdx(strIdx);
        int64 strSymIdx = CalculateStrLabelSymIdx(GlobalTables::GetGsymTable().GetSymbolTableSize(), strIdx);
        __ EmitDirectString(ustr, true, strSymIdx);
    }
}

void X64Emitter::EmitGlobalVariable(CG &cg)
{
    uint64 size = GlobalTables::GetGsymTable().GetSymbolTableSize();
    for (uint64 i = 0; i < size; ++i) {
        MIRSymbol *mirSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(i);

        if (mirSymbol == nullptr || mirSymbol->IsDeleted() || mirSymbol->GetStorageClass() == kScUnused) {
            continue;
        }

        MIRStorageClass storageClass = mirSymbol->GetStorageClass();
        /* symbols we do not emit here. */
        if (storageClass == kScTypeInfo || storageClass == kScTypeInfoName || storageClass == kScTypeCxxAbi) {
            continue;
        }

        MIRType *mirType = mirSymbol->GetType();
        if (mirType == nullptr) {
            continue;
        }
        int64 symIdx = mirSymbol->GetNameStrIdx().get();
        uint64 sizeInByte = GetSymbolSize(mirType->GetTypeIndex());
        uint8 alignInByte = GetSymbolAlign(*mirSymbol);

        /* Uninitialized global/static variables */
        if ((storageClass == kScGlobal || storageClass == kScFstatic) && !mirSymbol->IsConst()) {
            if (mirSymbol->IsGctibSym()) {
                continue;
            }
            __ StoreNameIntoSymMap(symIdx, mirSymbol->GetName());
            SectionKind secKind;
            if (mirSymbol->IsThreadLocal()) {
                secKind = kSTbss;
            } else if (maplebe::CGOptions::IsNoCommon()) {
                secKind = kSBss;
            } else {
                secKind = kSComm;
                alignInByte = GetSymbolAlign(*mirSymbol, true);
            }
            __ EmitVariable(symIdx, sizeInByte, alignInByte, kSAGlobal, secKind);
            continue;
        }
        MIRTypeKind kind = mirType->GetKind();
        /* Initialized global/static variables. */
        if (storageClass == kScGlobal || (storageClass == kScFstatic && !mirSymbol->IsReadOnly())) {
            MIRConst *mirConst = mirSymbol->GetKonst();
            uint64 valueSize = 0;
            __ StoreNameIntoSymMap(symIdx, mirSymbol->GetName());
            if (mirSymbol->IsThreadLocal()) {
                __ EmitVariable(symIdx, sizeInByte, alignInByte, kSAGlobal, kSTdata);
            } else {
                __ EmitVariable(symIdx, sizeInByte, alignInByte, kSAGlobal, kSData);
            }
            if (IsPrimitiveVector(mirType->GetPrimType())) {
                valueSize = EmitVector(*mirConst);
            } else if (IsPrimitiveScalar(mirType->GetPrimType())) {
                valueSize = EmitSingleElement(*mirConst, true, cg.GetMIRModule()->IsCModule());
            } else if (kind == kTypeArray) {
                CHECK_FATAL(!mirSymbol->HasAddrOfValues(), "EmitGlobalVariable: need EmitConstantTable");
                valueSize = EmitArray(*mirConst, cg);
            } else if (kind == kTypeStruct || kind == kTypeClass || kind == kTypeUnion) {
                CHECK_FATAL(!mirSymbol->HasAddrOfValues(), "EmitGlobalVariable: need EmitConstantTable");
                EmitStructure(*mirConst, cg);
            } else {
                DEBUG_ASSERT(false, "EmitGlobalVariable: Unknown mirKind");
            }
            __ PostEmitVariable(symIdx, kSAGlobal, valueSize);
        } else if (mirSymbol->IsReadOnly()) { /* If symbol is const & static */
            MIRConst *mirConst = mirSymbol->GetKonst();
            __ StoreNameIntoSymMap(symIdx, mirSymbol->GetName());
            if (mirConst == nullptr) {
                alignInByte = GetSymbolAlign(*mirSymbol, true);
                __ EmitVariable(symIdx, sizeInByte, alignInByte, kSAGlobal, kSComm);
            } else {
                SymbolAttr symAttr = kSAGlobal;
                if (mirSymbol->IsWeak()) {
                    symAttr = kSAWeak;
                } else if (storageClass == kScPstatic ||
                           (storageClass == kScFstatic && mirSymbol->sectionAttr == UStrIdx(0))) {
                    symAttr = kSAStatic;
                }
                __ EmitVariable(symIdx, sizeInByte, alignInByte, symAttr, kSRodata);
                if (IsPrimitiveVector(mirType->GetPrimType())) {
                    (void)EmitVector(*mirConst, false);
                } else if (IsPrimitiveScalar(mirType->GetPrimType())) {
                    if (storageClass == kScPstatic) {
                        (void)EmitSingleElement(*mirConst, false, true);
                    } else {
                        (void)EmitSingleElement(*mirConst, false);
                    }
                } else if (kind == kTypeArray) {
                    (void)EmitArray(*mirConst, cg, false);
                } else if (kind == kTypeStruct || kind == kTypeUnion || kind == kTypeClass) {
                    (void)EmitStructure(*mirConst, cg);
                } else {
                    FATAL(kLncFatal, "Unknown type in Global pstatic");
                }
            }
        }
    } /* end proccess all mirSymbols. */
    EmitStringPointers();
}

void X64Emitter::Run(CGFunc &cgFunc)
{
    X64CGFunc &x64CGFunc = static_cast<X64CGFunc &>(cgFunc);
    uint32 funcUniqueId = cgFunc.GetUniqueID();
    /* emit local variable(s) if exists */
    EmitLocalVariable(cgFunc);

    /* emit function header */
    EmitFunctionHeader(cgFunc);

    /* emit instructions */
    FOR_ALL_BB(bb, &x64CGFunc) {
        if (bb->IsUnreachable()) {
            continue;
        }

        /* emit bb headers */
        if (bb->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
            EmitBBHeaderLabel(cgFunc, bb->GetLabIdx(), bb->GetFrequency());
        }

        FOR_BB_INSNS(insn, bb) {
            EmitInsn(*insn, funcUniqueId);
        }
    }

    /* emit switch table if exists */
    EmitJmpTable(cgFunc);

    EmitFunctionFoot(cgFunc);

    __ ClearLocalSymMap();
}

bool CgEmission::PhaseRun(CGFunc &f)
{
    Emitter *emitter = f.GetCG()->GetEmitter();
    CHECK_NULL_FATAL(emitter);
    static_cast<X64Emitter *>(emitter)->Run(f);
    return false;
}

void X64Emitter::EmitDwFormAddr(const DBGDie &die, const DBGDieAttr &attr, DwAt attrName, DwTag tagName, DebugInfo &di)
{
    MapleVector<DBGDieAttr *> attrvec = die.GetAttrVec();
    if (attrName == static_cast<uint32>(DW_AT_low_pc) && tagName == static_cast<uint32>(DW_TAG_compile_unit)) {
        __ EmitDwFormAddr(true);
    }
    if (attrName == static_cast<uint32>(DW_AT_low_pc) && tagName == static_cast<uint32>(DW_TAG_subprogram)) {
        /* if decl, name should be found; if def, we try DW_AT_specification */
        DBGDieAttr *name = LFindAttribute(attrvec, static_cast<DwAt>(DW_AT_name));
        if (name == nullptr) {
            DBGDieAttr *spec = LFindAttribute(attrvec, static_cast<DwAt>(DW_AT_specification));
            CHECK_FATAL(spec != nullptr, "spec is null in Emitter::EmitDIAttrValue");
            DBGDie *decl = di.GetDie(spec->GetId());
            name = LFindAttribute(decl->GetAttrVec(), static_cast<DwAt>(DW_AT_name));
            CHECK_FATAL(name != nullptr, "name is null in Emitter::EmitDIAttrValue");
        }
        const std::string &str = GlobalTables::GetStrTable().GetStringFromStrIdx(name->GetId());
        MIRBuilder *mirbuilder = GetCG()->GetMIRModule()->GetMIRBuilder();
        MIRFunction *mfunc = mirbuilder->GetFunctionFromName(str);
        MapleMap<MIRFunction *, std::pair<LabelIdx, LabelIdx>>::iterator it = CG::GetFuncWrapLabels().find(mfunc);
        if (it != CG::GetFuncWrapLabels().end()) {
            /* it is a <pair> */
            __ EmitLabel(mfunc->GetPuidx(), (*it).second.first);
        } else {
            PUIdx pIdx = GetCG()->GetMIRModule()->CurFunction()->GetPuidx();
            __ EmitLabel(pIdx, attr.GetId()); /* maybe deadbeef */
        }
    }
    if (attrName == static_cast<uint32>(DW_AT_low_pc) && tagName == static_cast<uint32>(DW_TAG_label)) {
        DBGDie *subpgm = die.GetParent();
        DEBUG_ASSERT(subpgm->GetTag() == DW_TAG_subprogram, "Label DIE should be a child of a Subprogram DIE");
        DBGDieAttr *fnameAttr = LFindAttribute(subpgm->GetAttrVec(), static_cast<DwAt>(DW_AT_name));
        if (!fnameAttr) {
            DBGDieAttr *specAttr = LFindAttribute(subpgm->GetAttrVec(), static_cast<DwAt>(DW_AT_specification));
            CHECK_FATAL(specAttr, "pointer is null");
            DBGDie *twin = di.GetDie(static_cast<uint32>(specAttr->GetU()));
            fnameAttr = LFindAttribute(twin->GetAttrVec(), static_cast<DwAt>(DW_AT_name));
        }
    }
    if (attrName == static_cast<uint32>(DW_AT_high_pc)) {
        if (tagName == static_cast<uint32>(DW_TAG_compile_unit)) {
            __ EmitDwFormData8();
        }
    }
    if (attrName != static_cast<uint32>(DW_AT_high_pc) && attrName != static_cast<uint32>(DW_AT_low_pc)) {
        __ EmitDwFormAddr();
    }
}

void X64Emitter::EmitDwFormRef4(DBGDie &die, const DBGDieAttr &attr, DwAt attrName, DwTag tagName, DebugInfo &di)
{
    if (attrName == static_cast<uint32>(DW_AT_type)) {
        DBGDie *die0 = di.GetDie(static_cast<uint32>(attr.GetU()));
        if (die0->GetOffset()) {
            __ EmitDwFormRef4(die0->GetOffset());
        } else {
            /* unknown type, missing mplt */
            __ EmitDwFormRef4(di.GetDummyTypeDie()->GetOffset(), true);
        }
    } else if (attrName == static_cast<uint32>(DW_AT_specification) || attrName == static_cast<uint32>(DW_AT_sibling)) {
        DBGDie *die0 = di.GetDie(static_cast<uint32>(attr.GetU()));
        DEBUG_ASSERT(die0->GetOffset(), "");
        __ EmitDwFormRef4(die0->GetOffset());
    } else if (attrName == static_cast<uint32>(DW_AT_object_pointer)) {
        GStrIdx thisIdx = GlobalTables::GetStrTable().GetStrIdxFromName(kDebugMapleThis);
        DBGDie *that = LFindChildDieWithName(die, static_cast<DwTag>(DW_TAG_formal_parameter), thisIdx);
        /* need to find the this or self based on the source language
            what is the name for 'this' used in mapleir?
            this has to be with respect to a function */
        if (that) {
            __ EmitDwFormRef4(that->GetOffset());
        } else {
            __ EmitDwFormRef4(attr.GetU());
        }
    } else {
        __ EmitDwFormRef4(attr.GetU(), false, true);
    }
}

void X64Emitter::EmitDwFormData8(const DBGDieAttr &attr, DwAt attrName, DwTag tagName, DebugInfo &di,
                                 MapleVector<DBGDieAttr *> &attrvec)
{
    if (attrName == static_cast<uint32>(DW_AT_high_pc)) {
        if (tagName == static_cast<uint32>(DW_TAG_compile_unit)) {
            __ EmitDwFormData8();
        } else if (tagName == static_cast<uint32>(DW_TAG_subprogram)) {
            DBGDieAttr *name = LFindAttribute(attrvec, static_cast<DwAt>(DW_AT_name));
            if (name == nullptr) {
                DBGDieAttr *spec = LFindAttribute(attrvec, static_cast<DwAt>(DW_AT_specification));
                CHECK_FATAL(spec != nullptr, "spec is null in Emitter::EmitDIAttrValue");
                DBGDie *decl = di.GetDie(spec->GetId());
                name = LFindAttribute(decl->GetAttrVec(), static_cast<DwAt>(DW_AT_name));
                CHECK_FATAL(name != nullptr, "name is null in Emitter::EmitDIAttrValue");
            }
            const std::string &str = GlobalTables::GetStrTable().GetStringFromStrIdx(name->GetId());

            MIRBuilder *mirbuilder = GetCG()->GetMIRModule()->GetMIRBuilder();
            MIRFunction *mfunc = mirbuilder->GetFunctionFromName(str);
            MapleMap<MIRFunction *, std::pair<LabelIdx, LabelIdx>>::iterator it = CG::GetFuncWrapLabels().find(mfunc);
            uint32 endLabelFuncPuIdx;
            uint32 startLabelFuncPuIdx;
            uint32 endLabelIdx;
            uint32 startLabelIdx;
            if (it != CG::GetFuncWrapLabels().end()) {
                /* end label */
                endLabelFuncPuIdx = mfunc->GetPuidx();
                endLabelIdx = (*it).second.second;
            } else {
                /* maybe deadbeef */
                endLabelFuncPuIdx = GetCG()->GetMIRModule()->CurFunction()->GetPuidx();
                endLabelIdx = (*it).second.second;
            }
            if (it != CG::GetFuncWrapLabels().end()) {
                /* start label */
                startLabelFuncPuIdx = mfunc->GetPuidx();
                startLabelIdx = (*it).second.first;
            } else {
                DBGDieAttr *lowpc = LFindAttribute(attrvec, static_cast<DwAt>(DW_AT_low_pc));
                CHECK_FATAL(lowpc != nullptr, "lowpc is null in Emitter::EmitDIAttrValue");
                /* maybe deadbeef */
                startLabelFuncPuIdx = GetCG()->GetMIRModule()->CurFunction()->GetPuidx();
                startLabelIdx = lowpc->GetId();
            }
            __ EmitDwFormData8(endLabelFuncPuIdx, startLabelFuncPuIdx, endLabelIdx, startLabelIdx);
        }
    } else {
        __ EmitDwFormData(attr.GetI(), k8Bytes);
    }
}

void X64Emitter::EmitDIAttrValue(DBGDie &die, DBGDieAttr &attr, DwAt attrName, DwTag tagName, DebugInfo &di)
{
    MapleVector<DBGDieAttr *> &attrvec = die.GetAttrVec();
    switch (attr.GetDwForm()) {
        case DW_FORM_string:
            __ EmitDwFormString(GlobalTables::GetStrTable().GetStringFromStrIdx(attr.GetId()));
            break;
        case DW_FORM_strp:
            __ EmitDwFormStrp(attr.GetId(), GlobalTables::GetStrTable().StringTableSize());
            break;
        case DW_FORM_data1:
            __ EmitDwFormData(attr.GetI(), k1Byte);
            break;
        case DW_FORM_data2:
            __ EmitDwFormData(attr.GetI(), k2Bytes);
            break;
        case DW_FORM_data4:
            __ EmitDwFormData(attr.GetI(), k4Bytes);
            break;
        case DW_FORM_data8:
            EmitDwFormData8(attr, attrName, tagName, di, attrvec);
            break;
        case DW_FORM_sec_offset:
            if (attrName == static_cast<uint32>(DW_AT_stmt_list)) {
                __ EmitDwFormSecOffset();
            }
            break;
        case DW_FORM_addr:
            EmitDwFormAddr(die, attr, attrName, tagName, di);
            break;
        case DW_FORM_ref4:
            EmitDwFormRef4(die, attr, attrName, tagName, di);
            break;
        case DW_FORM_exprloc: {
            DBGExprLoc *elp = attr.GetPtr();
            switch (elp->GetOp()) {
                case DW_OP_call_frame_cfa:
                    __ EmitDwFormExprlocCfa(elp->GetOp());
                    break;
                case DW_OP_addr:
                    __ EmitDwFormExprlocAddr(elp->GetOp(),
                                             GlobalTables::GetStrTable()
                                                 .GetStringFromStrIdx(static_cast<uint32>(elp->GetGvarStridx()))
                                                 .c_str());
                    break;
                case DW_OP_fbreg:
                    __ EmitDwFormExprlocFbreg(elp->GetOp(), elp->GetFboffset(),
                                              namemangler::GetSleb128Size(elp->GetFboffset()));
                    break;
                case DW_OP_breg0:
                case DW_OP_breg1:
                case DW_OP_breg2:
                case DW_OP_breg3:
                case DW_OP_breg4:
                case DW_OP_breg5:
                case DW_OP_breg6:
                case DW_OP_breg7:
                    __ EmitDwFormExprlocBregn(elp->GetOp(), GetDwOpName(elp->GetOp()));
                    break;
                default:
                    __ EmitDwFormExprloc(uintptr(elp));
                    break;
            }
        } break;
        default:
            CHECK_FATAL(maple::GetDwFormName(attr.GetDwForm()) != nullptr,
                        "GetDwFormName return null in Emitter::EmitDIAttrValue");
            LogInfo::MapleLogger() << "unhandled : " << maple::GetDwFormName(attr.GetDwForm()) << std::endl;
            DEBUG_ASSERT(0, "NYI");
    }
}

void X64Emitter::EmitDIDebugInfoSection(DebugInfo &mirdi)
{
    __ EmitDIDebugInfoSectionHeader(mirdi.GetDebugInfoLength());
    /*
     * 7.5.1.2 type unit header
     * currently empty...
     *
     * 7.5.2 Debugging Information Entry (DIE)
     */
    X64Emitter *emitter = this;
    MapleVector<DBGAbbrevEntry *> &abbrevVec = mirdi.GetAbbrevVec();
    ApplyInPrefixOrder(mirdi.GetCompUnit(), [&abbrevVec, &emitter, &mirdi, this](DBGDie *die) {
        if (!die) {
            /* emit the null entry and return */
            emitter->GetAssembler().EmitDIDebugSectionEnd(kSDebugInfo);
            return;
        }
        bool verbose = emitter->GetCG()->GenerateVerboseAsm();
        if (verbose) {
            CHECK_FATAL(maple::GetDwTagName(die->GetTag()) != nullptr,
                        "GetDwTagName(die->GetTag()) return null in Emitter::EmitDIDebugInfoSection");
        }
        uint32 abbrevId = die->GetAbbrevId();
        emitter->GetAssembler().EmitDIDebugInfoSectionAbbrevId(verbose, abbrevId, maple::GetDwTagName(die->GetTag()),
                                                               die->GetOffset(), die->GetSize());
        DBGAbbrevEntry *diae = LFindAbbrevEntry(abbrevVec, abbrevId);
        CHECK_FATAL(diae != nullptr, "diae is null in Emitter::EmitDIDebugInfoSection");
        std::string sfile, spath;
        if (diae->GetTag() == static_cast<uint32>(DW_TAG_compile_unit) && sfile.empty()) {
            /* get full source path from fileMap[2] */
            if (emitter->GetFileMap().size() > k2ByteSize) { /* have src file map */
                std::string srcPath = emitter->GetFileMap()[k2ByteSize];
                size_t t = srcPath.rfind("/");
                DEBUG_ASSERT(t != std::string::npos, "");
                sfile = srcPath.substr(t + 1);
                spath = srcPath.substr(0, t);
            }
        }

        UpdateAttrAndEmit(sfile, mirdi, *diae, *die, spath);
    });
}

void X64Emitter::UpdateAttrAndEmit(const string &sfile, DebugInfo &mirdi, DBGAbbrevEntry &diae, DBGDie &die,
                                   const string &spath)
{
    X64Emitter *emitter = this;
    MapleVector<uint32> &apl = diae.GetAttrPairs(); /* attribute pair list */
    bool verbose = emitter->GetCG()->GenerateVerboseAsm();
    for (size_t i = 0; i < diae.GetAttrPairs().size(); i += k2ByteSize) {
        DBGDieAttr *attr = LFindAttribute(die.GetAttrVec(), DwAt(apl[i]));
        CHECK_FATAL(attr != nullptr, "attr is null");
        if (!LShouldEmit(unsigned(apl[i + 1]))) {
            continue;
        }

        /* update DW_AT_name and DW_AT_comp_dir attrs under DW_TAG_compile_unit
            to be C/C++ */
        if (!sfile.empty()) {
            if (attr->GetDwAt() == static_cast<uint32>(DW_AT_name)) {
                attr->SetId(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(sfile).GetIdx());
                emitter->GetCG()->GetMIRModule()->GetDbgInfo()->AddStrps(attr->GetId());
            } else if (attr->GetDwAt() == static_cast<uint32>(DW_AT_comp_dir)) {
                attr->SetId(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(spath).GetIdx());
                emitter->GetCG()->GetMIRModule()->GetDbgInfo()->AddStrps(attr->GetId());
            }
        }
        emitter->GetAssembler().EmitDIFormSpecification(unsigned(apl[i + 1]));
        emitter->EmitDIAttrValue(die, *attr, unsigned(apl[i]), diae.GetTag(), mirdi);
        if (verbose) {
            std::string dwAtName = maple::GetDwAtName(unsigned(apl[i]));
            std::string dwForName = maple::GetDwFormName(unsigned(apl[i + 1]));
            emitter->GetAssembler().EmitDIDwName(dwAtName, dwForName);
            if (apl[i + 1] == static_cast<uint32>(DW_FORM_strp) || apl[i + 1] == static_cast<uint32>(DW_FORM_string)) {
                emitter->GetAssembler().EmitDIDWFormStr(
                    GlobalTables::GetStrTable().GetStringFromStrIdx(attr->GetId()).c_str());
            } else if (apl[i] == static_cast<uint32>(DW_AT_data_member_location)) {
                emitter->GetAssembler().EmitDIDWDataMemberLocaltion(apl[i + 1], uintptr(attr));
            }
        }
        emitter->GetAssembler().EmitLine();
    }
}

void X64Emitter::EmitDIDebugAbbrevSection(DebugInfo &mirdi)
{
    __ EmitDIDebugAbbrevSectionHeader();

    /* construct a list of DI abbrev entries
       1. DW_TAG_compile_unit 0x11
       2. DW_TAG_subprogram   0x2e */
    bool verbose = GetCG()->GenerateVerboseAsm();
    for (DBGAbbrevEntry *diae : mirdi.GetAbbrevVec()) {
        if (!diae) {
            continue;
        }
        CHECK_FATAL(maple::GetDwTagName(diae->GetTag()) != nullptr,
                    "GetDwTagName return null in X64Emitter::EmitDIDebugAbbrevSection");
        __ EmitDIDebugAbbrevDiae(verbose, diae->GetAbbrevId(), diae->GetTag(), maple::GetDwTagName(diae->GetTag()),
                                 diae->GetWithChildren());

        MapleVector<uint32> &apl = diae->GetAttrPairs(); /* attribute pair list */

        for (size_t i = 0; i < diae->GetAttrPairs().size(); i += k2ByteSize) {
            CHECK_FATAL(maple::GetDwAtName(unsigned(apl[i])) != nullptr,
                        "GetDwAtName return null in X64Emitter::EmitDIDebugAbbrevSection");
            CHECK_FATAL(maple::GetDwFormName(unsigned(apl[i + 1])) != nullptr,
                        "GetDwFormName return null in X64Emitter::EmitDIDebugAbbrevSection");
            __ EmitDIDebugAbbrevDiaePairItem(verbose, apl[i], apl[1 + 1], maple::GetDwAtName(unsigned(apl[i])),
                                             maple::GetDwFormName(unsigned(apl[i + 1])));
        }
        __ EmitDIDebugSectionEnd(kSDebugAbbrev);
        __ EmitDIDebugSectionEnd(kSDebugAbbrev);
    }
    __ EmitDIDebugSectionEnd(kSDebugAbbrev);
}

void X64Emitter::EmitDIDebugStrSection()
{
    std::vector<std::string> debugStrs;
    std::vector<uint32> strps;
    for (auto it : GetCG()->GetMIRModule()->GetDbgInfo()->GetStrps()) {
        const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(it);
        (void)debugStrs.emplace_back(name);
        (void)strps.emplace_back(it);
    }
    __ EmitDIDebugStrSection(strps, debugStrs, GlobalTables::GetGsymTable().GetSymbolTableSize(),
                             GlobalTables::GetStrTable().StringTableSize());
}

void X64Emitter::EmitDebugInfo(CG &cg)
{
    if (!cg.GetCGOptions().WithDwarf()) {
        return;
    }
    SetupDBGInfo(cg.GetMIRModule()->GetDbgInfo());
    __ EmitDIHeaderFileInfo();
    EmitDIDebugInfoSection(*(cg.GetMIRModule()->GetDbgInfo()));
    EmitDIDebugAbbrevSection(*(cg.GetMIRModule()->GetDbgInfo()));
    __ EmitDIDebugARangesSection();
    __ EmitDIDebugRangesSection();
    __ EmitDIDebugLineSection();
    EmitDIDebugStrSection();
}

MAPLE_TRANSFORM_PHASE_REGISTER(CgEmission, cgemit)
} /* namespace maplebe */
