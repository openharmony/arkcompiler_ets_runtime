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

#include "aarch64_emitter.h"
#include <sys/stat.h>
#include "aarch64_cgfunc.h"
#include "aarch64_cg.h"
#include "metadata_layout.h"
#include "cfi.h"
#include "aarch64_obj_emitter.h"
#include "operand.h"

namespace {
using namespace maple;
}  // namespace

namespace maplebe {
using namespace maple;

void AArch64AsmEmitter::EmitBBHeaderLabel(FuncEmitInfo &funcEmitInfo, const std::string &name, LabelIdx labIdx)
{
#ifdef ARK_LITECG_DEBUG
    (void)name;
    CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
    AArch64CGFunc &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();
    LabelOperand &label = aarchCGFunc.GetOrCreateLabelOperand(labIdx);
    /* if label order is default value -1, set new order */
    if (label.GetLabelOrder() == 0xFFFFFFFF) {
        label.SetLabelOrder(currCG->GetLabelOrderCnt());
        currCG->IncreaseLabelOrderCnt();
    }
    CHECK_NULL_FATAL(currCG->GetMIRModule()->CurFunction());
    PUIdx pIdx = currCG->GetMIRModule()->CurFunction()->GetPuidx();
    char *puIdx = strdup(std::to_string(pIdx).c_str());
    CHECK_FATAL(puIdx != nullptr, "strdup failed");
    currCG->template Emit<CG::EmitterType::AsmEmitter>([&cgFunc, currCG, puIdx, labIdx, label](Emitter* emitter) {
        const std::string &labelName = cgFunc.GetFunction().GetLabelTab()->GetName(labIdx);
        if (currCG->GenerateVerboseCG()) {
            (void)emitter->Emit(".L.")
                .Emit(puIdx)
                .Emit("__")
                .Emit(labIdx)
                .Emit(":\t//label order ")
                .Emit(label.GetLabelOrder());
            if (!labelName.empty() && labelName.at(0) != '@') {
                /* If label name has @ as its first char, it is not from MIR */
                (void)emitter->Emit(", MIR: @").Emit(labelName).Emit("\n");
            } else {
                (void)emitter->Emit("\n");
            }
        } else {
            (void)emitter->Emit(".L.").Emit(puIdx).Emit("__").Emit(labIdx).Emit(":\n");
        }
    });
    free(puIdx);
    puIdx = nullptr;
#endif
}

void AArch64AsmEmitter::RecordRegInfo(FuncEmitInfo &funcEmitInfo) const
{
#ifdef ARK_LITECG_DEBUG
    if (!CGOptions::DoIPARA()) {
        return;
    }
    CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
    AArch64CGFunc &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);

    std::set<regno_t> referedRegs;
    MIRFunction &mirFunc = cgFunc.GetFunction();
    FOR_ALL_BB_REV(bb, &aarchCGFunc)
    {
        FOR_BB_INSNS_REV(insn, bb)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            if (insn->IsCall() || insn->IsTailCall()) {
                auto *targetOpnd = insn->GetCallTargetOperand();
                bool safeCheck = false;
                CHECK_FATAL(targetOpnd != nullptr,
                            "target is null in AArch64Emitter::IsCallToFunctionThatNeverReturns");
                if (targetOpnd->IsFuncNameOpnd()) {
                    FuncNameOperand *target = static_cast<FuncNameOperand *>(targetOpnd);
                    const MIRSymbol *funcSt = target->GetFunctionSymbol();
                    DEBUG_ASSERT(funcSt->GetSKind() == maple::kStFunc, "funcst must be a function name symbol");
                    MIRFunction *func = funcSt->GetFunction();
                    if (func != nullptr && func->IsReferedRegsValid()) {
                        safeCheck = true;
                        for (auto preg : func->GetReferedRegs()) {
                            referedRegs.insert(preg);
                        }
                    }
                }
                if (!safeCheck) {
                    mirFunc.SetReferedRegsValid(false);
                    return;
                }
            }
            if (referedRegs.size() == kMaxRegNum) {
                break;
            }
            uint32 opndNum = insn->GetOperandSize();
            const InsnDesc *md = &AArch64CG::kMd[insn->GetMachineOpcode()];
            for (uint32 i = 0; i < opndNum; ++i) {
                if (insn->GetMachineOpcode() == MOP_asm) {
                    if (i == kAsmOutputListOpnd || i == kAsmClobberListOpnd) {
                        for (auto opnd : static_cast<ListOperand &>(insn->GetOperand(i)).GetOperands()) {
                            if (opnd->IsRegister()) {
                                referedRegs.insert(static_cast<RegOperand *>(opnd)->GetRegisterNumber());
                            }
                        }
                    }
                    continue;
                }
                Operand &opnd = insn->GetOperand(i);
                if (opnd.IsList()) {
                    /* all use, skip it */
                } else if (opnd.IsMemoryAccessOperand()) {
                    auto &memOpnd = static_cast<MemOperand &>(opnd);
                    RegOperand *base = memOpnd.GetBaseRegister();
                    if (!memOpnd.IsIntactIndexed()) {
                        referedRegs.insert(base->GetRegisterNumber());
                    }
                } else if (opnd.IsRegister()) {
                    RegType regType = static_cast<RegOperand &>(opnd).GetRegisterType();
                    if (regType == kRegTyCc || regType == kRegTyVary) {
                        continue;
                    }
                    bool isDef = md->GetOpndDes(i)->IsRegDef();
                    if (isDef) {
                        referedRegs.insert(static_cast<RegOperand &>(opnd).GetRegisterNumber());
                    }
                }
            }
        }
    }
    mirFunc.SetReferedRegsValid(true);
#ifdef DEBUG
    for (auto reg : referedRegs) {
        if (reg > kMaxRegNum) {
            DEBUG_ASSERT(0, "unexpected preg");
        }
    }
#endif
    mirFunc.CopyReferedRegs(referedRegs);
#endif
}

void AArch64AsmEmitter::Run(FuncEmitInfo &funcEmitInfo)
{
#ifdef ARK_LITECG_DEBUG
    CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
    AArch64CGFunc &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();
    /* emit header of this function */
    currCG->template Emit<CG::EmitterType::AsmEmitter>(
    [this, &cgFunc, &aarchCGFunc, currCG, &funcEmitInfo](Emitter *emitter) {
        // insert for  __cxx_global_var_init
        if (cgFunc.GetName() == "__cxx_global_var_init") {
            (void)emitter->Emit("\t.section\t.init_array,\"aw\"\n");
            (void)emitter->Emit("\t.quad\t").Emit(cgFunc.GetName()).Emit("\n");
        }
        if (cgFunc.GetFunction().GetAttr(FUNCATTR_initialization)) {
            (void)emitter->Emit("\t.section\t.init_array,\"aw\"\n");
            (void)emitter->Emit("\t.quad\t").Emit(cgFunc.GetName()).Emit("\n");
        }
        if (cgFunc.GetFunction().GetAttr(FUNCATTR_termination)) {
            (void)emitter->Emit("\t.section\t.fini_array,\"aw\"\n");
            (void)emitter->Emit("\t.quad\t").Emit(cgFunc.GetName()).Emit("\n");
        }
        (void)emitter->Emit("\n");
        if (cgFunc.GetFunction().GetAttr(FUNCATTR_section)) {
            const std::string &sectionName = cgFunc.GetFunction().GetAttrs().GetPrefixSectionName();
            (void)emitter->Emit("\t.section  " + sectionName).Emit(",\"ax\",@progbits\n");
        } else if (cgFunc.GetFunction().GetAttr(FUNCATTR_constructor_priority)) {
            (void)emitter->Emit("\t.section\t.text.startup").Emit(",\"ax\",@progbits\n");
        } else {
            (void)emitter->Emit("\t.text\n");
        }
        if (CGOptions::GetFuncAlignPow() != 0) {
            (void)emitter->Emit("\t.align ").Emit(CGOptions::GetFuncAlignPow()).Emit("\n");
        }
        MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(cgFunc.GetFunction().GetStIdx().Idx());
        const std::string &funcName = std::string(cgFunc.GetShortFuncName().c_str());

        CHECK_NULL_FATAL(funcSt);
        std::string funcStName = funcSt->GetName();
        if (funcSt->GetFunction()->GetAttr(FUNCATTR_weak)) {
            (void)emitter->Emit("\t.weak\t" + funcStName + "\n");
            (void)emitter->Emit("\t.hidden\t" + funcStName + "\n");
        } else if (funcSt->GetFunction()->GetAttr(FUNCATTR_local)) {
            (void)emitter->Emit("\t.local\t" + funcStName + "\n");
        } else if (funcSt->GetFunction() && funcSt->GetFunction()->IsStatic()) {
            // nothing
        } else {
            /* should refer to function attribute */
            (void)emitter->Emit("\t.globl\t").Emit(funcSt->GetName()).Emit("\n");
            (void)emitter->Emit("\t.hidden\t").Emit(funcSt->GetName()).Emit("\n");
        }
        (void)emitter->Emit("\t.type\t" + funcStName + ", %function\n");
        (void)emitter->Emit(funcStName + ":\n");

        /* if the last  insn is call, then insert nop */
        bool found = false;
        FOR_ALL_BB_REV(bb, &aarchCGFunc)
        {
            FOR_BB_INSNS_REV(insn, bb)
            {
                if (insn->IsMachineInstruction()) {
                    if (insn->IsCall()) {
                        Insn &newInsn = aarchCGFunc.GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_nop);
                        bb->InsertInsnAfter(*insn, newInsn);
                    }
                    found = true;
                    break;
                }
            }
            if (found) {
                break;
            }
        }

        RecordRegInfo(funcEmitInfo);

        /* emit instructions */
        FOR_ALL_BB(bb, &aarchCGFunc)
        {
            if (bb->IsUnreachable()) {
                continue;
            }
            if (currCG->GenerateVerboseCG()) {
                (void)emitter->Emit("#    freq:").Emit(bb->GetFrequency()).Emit("\n");
            }
            /* emit bb headers */
            if (bb->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
                EmitBBHeaderLabel(funcEmitInfo, funcName, bb->GetLabIdx());
            }

            FOR_BB_INSNS(insn, bb)
            {
                auto dbgComment = insn->GetDebugComment();
                if (currDebugComment != dbgComment) {
                    currDebugComment = dbgComment;
                    if (dbgComment != nullptr) {
                        emitter->Emit("\t// ");
                        emitter->Emit(dbgComment->c_str());
                        emitter->Emit("\n ");
                    } else {
                        emitter->Emit("\t// \n");
                    }
                }
                if (insn->IsCfiInsn()) {
                    EmitAArch64CfiInsn(*emitter, *insn);
                } else {
                    EmitAArch64Insn(*emitter, *insn);
                }
            }
        }
        (void)emitter->Emit("\t.size\t" + funcStName + ", .-").Emit(funcStName + "\n");

        auto constructorAttr = funcSt->GetFunction()->GetAttrs().GetConstructorPriority();
        if (constructorAttr != -1) {
            (void)emitter->Emit("\t.section\t.init_array." + std::to_string(constructorAttr) + ",\"aw\"\n");
            (void)emitter->Emit("\t.align 3\n");
            (void)emitter->Emit("\t.xword\t" + funcStName + "\n");
        }

        EmitFunctionSymbolTable(funcEmitInfo);

        for (auto &it : cgFunc.GetEmitStVec()) {
            /* emit switch table only here */
            MIRSymbol *st = it.second;
            DEBUG_ASSERT(st->IsReadOnly(), "NYI");
            (void)emitter->Emit("\n");
            (void)emitter->Emit("\t.align 3\n");
            (void)emitter->Emit(st->GetName() + ":\n");
            MIRAggConst *arrayConst = safe_cast<MIRAggConst>(st->GetKonst());
            CHECK_NULL_FATAL(cgFunc.GetMirModule().CurFunction());
            PUIdx pIdx = cgFunc.GetMirModule().CurFunction()->GetPuidx();
            char *idx = strdup(std::to_string(pIdx).c_str());
            CHECK_FATAL(arrayConst != nullptr, "null ptr check");
            for (size_t i = 0; i < arrayConst->GetConstVec().size(); i++) {
                MIRLblConst *lblConst = safe_cast<MIRLblConst>(arrayConst->GetConstVecItem(i));
                CHECK_FATAL(lblConst != nullptr, "null ptr check");
                CHECK_FATAL(idx != nullptr, "strdup failed");
                (void)emitter->Emit("\t.quad\t.L.").Emit(idx).Emit("__").Emit(lblConst->GetValue());
                (void)emitter->Emit(" - " + st->GetName() + "\n");
            }
            free(idx);
            idx = nullptr;
        }

        for (const auto &mpPair : cgFunc.GetLabelAndValueMap()) {
            LabelOperand &labelOpnd = aarchCGFunc.GetOrCreateLabelOperand(mpPair.first);
            A64OpndEmitVisitor visitor(*emitter, nullptr);
            labelOpnd.Accept(visitor);
            (void)emitter->Emit(":\n");
            (void)emitter->Emit("\t.quad ").Emit(static_cast<int64>(mpPair.second)).Emit("\n");
        }
    });
#endif
}

void AArch64AsmEmitter::EmitAArch64Insn(maplebe::Emitter &emitter, Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    MOperator mOp = insn.GetMachineOpcode();
    emitter.SetCurrentMOP(mOp);
    const InsnDesc *md = insn.GetDesc();

    if (!GetCG()->GenerateVerboseAsm() && !GetCG()->GenerateVerboseCG() && insn.IsComment()) {
        return;
    }

    switch (mOp) {
        case MOP_adrp_ldr: {
            EmitAdrpLdr(emitter, insn);
            if (CGOptions::IsLazyBinding() && !GetCG()->IsLibcore()) {
                EmitLazyBindingRoutine(emitter, insn);
            }
            return;
        }
        case MOP_counter: {
            EmitCounter(emitter, insn);
            return;
        }
        case MOP_asm: {
            EmitInlineAsm(emitter, insn);
            return;
        }
        case MOP_clinit_tail: {
            EmitClinitTail(emitter, insn);
            return;
        }
        case MOP_lazy_ldr: {
            EmitLazyLoad(emitter, insn);
            return;
        }
        case MOP_adrp_label: {
            EmitAdrpLabel(emitter, insn);
            return;
        }
        case MOP_lazy_tail: {
            /* No need to emit this pseudo instruction. */
            return;
        }
        case MOP_lazy_ldr_static: {
            EmitLazyLoadStatic(emitter, insn);
            return;
        }
        case MOP_arrayclass_cache_ldr: {
            EmitArrayClassCacheLoad(emitter, insn);
            return;
        }
        case MOP_get_and_addI:
        case MOP_get_and_addL: {
            EmitGetAndAddInt(emitter, insn);
            return;
        }
        case MOP_get_and_setI:
        case MOP_get_and_setL: {
            EmitGetAndSetInt(emitter, insn);
            return;
        }
        case MOP_compare_and_swapI:
        case MOP_compare_and_swapL: {
            EmitCompareAndSwapInt(emitter, insn);
            return;
        }
        case MOP_string_indexof: {
            EmitStringIndexOf(emitter, insn);
            return;
        }
        case MOP_pseudo_none:
        case MOP_pseduo_tls_release: {
            return;
        }
        case MOP_tls_desc_call: {
            EmitCTlsDescCall(emitter, insn);
            return;
        }
        case MOP_tls_desc_rel: {
            EmitCTlsDescRel(emitter, insn);
            return;
        }
        case MOP_sync_lock_test_setI:
        case MOP_sync_lock_test_setL: {
            EmitSyncLockTestSet(emitter, insn);
            return;
        }
        case MOP_pure_call: {
            EmitPureCall(emitter, insn);
            return;
        }
        default:
            break;
    }

    std::string format(md->format);
    (void)emitter.Emit("\t").Emit(md->name).Emit("\t");
    size_t opndSize = insn.GetOperandSize();
    std::vector<int32> seq(opndSize, -1);
    std::vector<std::string> prefix(opndSize); /* used for print prefix like "*" in icall *rax */
    uint32 index = 0;
    uint32 commaNum = 0;
    for (uint32 i = 0; i < format.length(); ++i) {
        char c = format[i];
        if (c >= '0' && c <= '5') {
            seq[index++] = c - '0';
            ++commaNum;
        } else if (c != ',') {
            prefix[index].push_back(c);
        }
    }

    bool isRefField =
        (opndSize == 0) ? false : CheckInsnRefField(insn, static_cast<size_t>(static_cast<uint32>(seq[0])));
    uint32 compositeOpnds = 0;
    for (uint32 i = 0; i < commaNum; ++i) {
        if (seq[i] == -1) {
            continue;
        }
        if (prefix[i].length() > 0) {
            (void)emitter.Emit(prefix[i]);
        }
        if (emitter.NeedToDealWithHugeSo() && (mOp == MOP_xbl || mOp == MOP_tail_call_opt_xbl)) {
            auto *nameOpnd = static_cast<FuncNameOperand *>(&insn.GetOperand(kInsnFirstOpnd));
            emitter.InsertHugeSoTarget(nameOpnd->GetName());
            (void)emitter.Emit(nameOpnd->GetName() + emitter.HugeSoPostFix());
            break;
        }
        A64OpndEmitVisitor visitor(emitter, md->opndMD[static_cast<uint32>(seq[i])]);

        insn.GetOperand(static_cast<uint32>(seq[i])).Accept(visitor);
        if (compositeOpnds == 1) {
            (void)emitter.Emit("}");
        }
        if (compositeOpnds > 0) {
            --compositeOpnds;
        }
        /* reset opnd0 ref-field flag, so following instruction has correct register */
        if (isRefField && (i == 0)) {
            static_cast<RegOperand *>(&insn.GetOperand(static_cast<uint32>(seq[0])))->SetRefField(false);
        }
        /* Temporary comment the label:.Label.debug.callee */
        if (i != (commaNum - 1)) {
            (void)emitter.Emit(", ");
        }
        const uint32 commaNumForEmitLazy = 2;
        if (!CGOptions::IsLazyBinding() || GetCG()->IsLibcore() || (mOp != MOP_wldr && mOp != MOP_xldr) ||
            commaNum != commaNumForEmitLazy || i != 1 ||
            !insn.GetOperand(static_cast<uint32>(seq[1])).IsMemoryAccessOperand()) {
            continue;
        }
        /*
         * Only check the last operand of ldr in lo12 mode.
         * Check the second operand, if it's [AArch64MemOperand::kAddrModeLo12Li]
         */
        auto *memOpnd = static_cast<MemOperand *>(&insn.GetOperand(static_cast<uint32>(seq[1])));
        if (memOpnd == nullptr || memOpnd->GetAddrMode() != MemOperand::kAddrModeLo12Li) {
            continue;
        }
        const MIRSymbol *sym = memOpnd->GetSymbol();
        if (sym->IsMuidFuncDefTab() || sym->IsMuidFuncUndefTab() || sym->IsMuidDataDefTab() ||
            sym->IsMuidDataUndefTab()) {
            (void)emitter.Emit("\n");
            EmitLazyBindingRoutine(emitter, insn);
        }
    }
    if (GetCG()->GenerateVerboseCG() || (GetCG()->GenerateVerboseAsm() && insn.IsComment())) {
        const char *comment = insn.GetComment().c_str();
        if (comment != nullptr && strlen(comment) > 0) {
            (void)emitter.Emit("\t\t// ").Emit(comment);
        }
    }

    (void)emitter.Emit("\n");
#endif
}

#ifdef ARK_LITECG_DEBUG
static void AsmStringOutputRegNum(bool isInt, uint32 regno, uint32 intBase, uint32 fpBase, std::string &strToEmit)
{
    regno_t newRegno;
    if (isInt) {
        CHECK_FATAL(regno >= intBase, "value overflow");
        newRegno = regno - intBase;
    } else {
        newRegno = regno - fpBase;
    }
    if (newRegno > (kDecimalMax - 1)) {
        uint32 tenth = newRegno / kDecimalMax;
        strToEmit += '0' + static_cast<char>(tenth);
        newRegno -= (kDecimalMax * tenth);
    }
    strToEmit += newRegno + '0';
}
#endif

void AArch64AsmEmitter::EmitInlineAsm(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    (void)emitter.Emit("\t//Inline asm begin\n\t");
    auto &list1 = static_cast<ListOperand &>(insn.GetOperand(kAsmOutputListOpnd));
    std::vector<RegOperand *> outOpnds;
    for (auto *regOpnd : list1.GetOperands()) {
        outOpnds.push_back(regOpnd);
    }
    auto &list2 = static_cast<ListOperand &>(insn.GetOperand(kAsmInputListOpnd));
    std::vector<RegOperand *> inOpnds;
    for (auto *regOpnd : list2.GetOperands()) {
        inOpnds.push_back(regOpnd);
    }
    auto &list6 = static_cast<ListConstraintOperand &>(insn.GetOperand(kAsmOutputRegPrefixOpnd));
    auto &list7 = static_cast<ListConstraintOperand &>(insn.GetOperand(kAsmInputRegPrefixOpnd));
    MapleString asmStr = static_cast<StringOperand &>(insn.GetOperand(kAsmStringOpnd)).GetComment();
    std::string stringToEmit;
    auto IsMemAccess = [](char c) -> bool { return c == '['; };
    auto EmitRegister = [&](const char *p, bool isInt, uint32 regNO, bool unDefRegSize) -> void {
        if (IsMemAccess(p[0])) {
            stringToEmit += "[x";
            AsmStringOutputRegNum(isInt, regNO, R0, V0, stringToEmit);
            stringToEmit += "]";
        } else {
            DEBUG_ASSERT((p[0] == 'w' || p[0] == 'x' || p[0] == 's' || p[0] == 'd' || p[0] == 'v'),
                         "Asm invalid register type");
            if ((p[0] == 'w' || p[0] == 'x') && unDefRegSize) {
                stringToEmit += 'x';
            } else {
                stringToEmit += p[0];
            }
            if (!unDefRegSize) {
                isInt = (p[0] == 'w' || p[0] == 'x');
            }
            AsmStringOutputRegNum(isInt, regNO, R0, V0, stringToEmit);
        }
    };
    for (size_t i = 0; i < asmStr.length(); ++i) {
        switch (asmStr[i]) {
            case '$': {
                char c = asmStr[++i];
                if ((c >= '0') && (c <= '9')) {
                    auto val = static_cast<uint32>(c - '0');
                    if (asmStr[i + 1] >= '0' && asmStr[i + 1] <= '9') {
                        val = val * kDecimalMax + static_cast<uint32>(asmStr[++i] - '0');
                    }
                    if (val < outOpnds.size()) {
                        const char *prefix = list6.stringList[val]->GetComment().c_str();
                        RegOperand *opnd = outOpnds[val];
                        EmitRegister(prefix, opnd->IsOfIntClass(), opnd->GetRegisterNumber(), true);
                    } else {
                        val -= static_cast<uint32>(outOpnds.size());
                        CHECK_FATAL(val < inOpnds.size(), "Inline asm : invalid register constraint number");
                        RegOperand *opnd = inOpnds[val];
                        /* input is a immediate */
                        const char *prefix = list7.stringList[val]->GetComment().c_str();
                        if (prefix[0] == 'i') {
                            stringToEmit += '#';
                            for (size_t k = 1; k < list7.stringList[val]->GetComment().length(); ++k) {
                                stringToEmit += prefix[k];
                            }
                        } else {
                            EmitRegister(prefix, opnd->IsOfIntClass(), opnd->GetRegisterNumber(), true);
                        }
                    }
                } else if (c == '{') {
                    c = asmStr[++i];
                    CHECK_FATAL(((c >= '0') && (c <= '9')), "Inline asm : invalid register constraint number");
                    auto val = static_cast<uint32>(c - '0');
                    if (asmStr[i + 1] >= '0' && asmStr[i + 1] <= '9') {
                        val = val * kDecimalMax + static_cast<uint32>(asmStr[++i] - '0');
                    }
                    regno_t regno;
                    bool isAddr = false;
                    if (val < outOpnds.size()) {
                        RegOperand *opnd = outOpnds[val];
                        regno = opnd->GetRegisterNumber();
                        isAddr = IsMemAccess(list6.stringList[val]->GetComment().c_str()[0]);
                    } else {
                        val -= static_cast<uint32>(outOpnds.size());
                        CHECK_FATAL(val < inOpnds.size(), "Inline asm : invalid register constraint number");
                        RegOperand *opnd = inOpnds[val];
                        regno = opnd->GetRegisterNumber();
                        DEBUG_ASSERT(list7.stringList[val]->GetComment().c_str() != nullptr,
                            "list7 GetComment.c_str should not be nullptr");
                        isAddr = IsMemAccess(list7.stringList[val]->GetComment().c_str()[0]);
                    }
                    c = asmStr[++i];
                    CHECK_FATAL(c == ':', "Parsing error in inline asm string during emit");
                    c = asmStr[++i];
                    std::string prefix(1, c);
                    if (c == 'a' || isAddr) {
                        prefix = "[x";
                    }
                    EmitRegister(prefix.c_str(), true, regno, false);
                    c = asmStr[++i];
                    CHECK_FATAL(c == '}', "Parsing error in inline asm string during emit");
                }
                break;
            }
            case '\n': {
                stringToEmit += "\n\t";
                break;
            }
            default:
                stringToEmit += asmStr[i];
        }
    }
    (void)emitter.Emit(stringToEmit);
    (void)emitter.Emit("\n\t//Inline asm end\n");
#endif
}

void AArch64AsmEmitter::EmitClinitTail(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    /*
     * ldr x17, [xs, #112]
     * ldr wzr, [x17]
     */
    const InsnDesc *md = &AArch64CG::kMd[MOP_clinit_tail];

    Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);

    const OpndDesc *prop0 = md->opndMD[0];
    A64OpndEmitVisitor visitor(emitter, prop0);

    /* emit "ldr  x17,[xs,#112]" */
    (void)emitter.Emit("\t").Emit("ldr").Emit("\tx17, [");
    opnd0->Accept(visitor);
    (void)emitter.Emit(", #");
    (void)emitter.Emit(static_cast<uint32>(ClassMetadata::OffsetOfInitState()));
    (void)emitter.Emit("]");
    (void)emitter.Emit("\n");

    /* emit "ldr  xzr, [x17]" */
    (void)emitter.Emit("\t").Emit("ldr\txzr, [x17]\n");
#endif
}

void AArch64AsmEmitter::EmitLazyLoad(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    /*
     * ldr wd, [xs]  # xd and xs should be differenct register
     * ldr wd, [xd]
     */
    const InsnDesc *md = &AArch64CG::kMd[MOP_lazy_ldr];

    Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
    Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
    const OpndDesc *prop0 = md->opndMD[0];
    const OpndDesc *prop1 = md->opndMD[1];
    A64OpndEmitVisitor visitor(emitter, prop0);
    A64OpndEmitVisitor visitor1(emitter, prop1);

    /* emit  "ldr wd, [xs]" */
    (void)emitter.Emit("\t").Emit("ldr\t");
#ifdef USE_32BIT_REF
    opnd0->Accept(visitor);
#else
    opnd0->Accept(visitor1);
#endif
    (void)emitter.Emit(", [");
    opnd1->Accept(visitor1);
    (void)emitter.Emit("]\t// lazy load.\n");

    /* emit "ldr wd, [xd]" */
    (void)emitter.Emit("\t").Emit("ldr\t");
    opnd0->Accept(visitor);
    (void)emitter.Emit(", [");
    opnd1->Accept(visitor1);
    (void)emitter.Emit("]\t// lazy load.\n");
#endif
}

void AArch64AsmEmitter::EmitCounter(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    /*
     * adrp    x1, __profile_bb_table$$GetBoolean_bytecode+4
     * ldr     w17, [x1, #:lo12:__profile_bb_table$$GetBoolean_bytecode+4]
     * add     w17, w17, #1
     * str     w17, [x1, #:lo12:__profile_bb_table$$GetBoolean_bytecode+4]
     */
    const InsnDesc *md = &AArch64CG::kMd[MOP_counter];

    Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
    Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
    const OpndDesc *prop0 = md->opndMD[kInsnFirstOpnd];
    A64OpndEmitVisitor visitor(emitter, prop0);
    StImmOperand *stImmOpnd = static_cast<StImmOperand *>(opnd1);
    CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Emitter::EmitCounter");
    /* emit adrp */
    (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
    opnd0->Accept(visitor);
    (void)emitter.Emit(",");
    (void)emitter.Emit(stImmOpnd->GetName());
    (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
    (void)emitter.Emit("\n");
    /* emit ldr */
    (void)emitter.Emit("\t").Emit("ldr").Emit("\tw17, [");
    opnd0->Accept(visitor);
    (void)emitter.Emit(",");
    (void)emitter.Emit("#");
    (void)emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
    (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
    (void)emitter.Emit("]");
    (void)emitter.Emit("\n");
    /* emit add */
    (void)emitter.Emit("\t").Emit("add").Emit("\tw17, w17, #1");
    (void)emitter.Emit("\n");
    /* emit str */
    (void)emitter.Emit("\t").Emit("str").Emit("\tw17, [");
    opnd0->Accept(visitor);
    (void)emitter.Emit(",");
    (void)emitter.Emit("#");
    (void)emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
    (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
    (void)emitter.Emit("]");
    (void)emitter.Emit("\n");
#endif
}

void AArch64AsmEmitter::EmitAdrpLabel(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    /* adrp    xd, label
     * add     xd, xd, #lo12:label
     */
    const InsnDesc *md = &AArch64CG::kMd[MOP_adrp_label];

    Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
    Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
    const OpndDesc *prop0 = md->opndMD[0];
    A64OpndEmitVisitor visitor(emitter, prop0);
    auto lidx = static_cast<ImmOperand *>(opnd1)->GetValue();

    /* adrp    xd, label */
    (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
    opnd0->Accept(visitor);
    (void)emitter.Emit(", ");
    char *idx;
    CHECK_NULL_FATAL(Globals::GetInstance()->GetBECommon()->GetMIRModule().CurFunction());
    idx =
        strdup(std::to_string(Globals::GetInstance()->GetBECommon()->GetMIRModule().CurFunction()->GetPuidx()).c_str());
    CHECK_FATAL(idx != nullptr, "strdup failed");
    (void)emitter.Emit(".L.").Emit(idx).Emit("__").Emit(lidx).Emit("\n");

    /* add     xd, xd, #lo12:label */
    (void)emitter.Emit("\tadd\t");
    opnd0->Accept(visitor);
    (void)emitter.Emit(", ");
    opnd0->Accept(visitor);
    (void)emitter.Emit(", ");
    (void)emitter.Emit(":lo12:").Emit(".L.").Emit(idx).Emit("__").Emit(lidx).Emit("\n");
    (void)emitter.Emit("\n");
    free(idx);
    idx = nullptr;
#endif
}

void AArch64AsmEmitter::EmitAdrpLdr(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    const InsnDesc *md = &AArch64CG::kMd[MOP_adrp_ldr];
    Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
    Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
    const OpndDesc *prop0 = md->opndMD[0];
    A64OpndEmitVisitor visitor(emitter, prop0);
    auto *stImmOpnd = static_cast<StImmOperand *>(opnd1);
    CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Emitter::EmitAdrpLdr");

    (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
    opnd0->Accept(visitor);
    (void)emitter.Emit(", ");
    (void)emitter.Emit(stImmOpnd->GetName());
    if (stImmOpnd->GetOffset() != 0) {
        (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
    }
    (void)emitter.Emit("\n");

    (void)emitter.Emit("\tldr\t");
    static_cast<RegOperand *>(opnd0)->SetRefField(true);
    opnd0->Accept(visitor);
    static_cast<RegOperand *>(opnd0)->SetRefField(false);
    (void)emitter.Emit(", ");
    (void)emitter.Emit("[");
    opnd0->Accept(visitor);
    (void)emitter.Emit(",");
    (void)emitter.Emit("#");
    (void)emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
    if (stImmOpnd->GetOffset() != 0) {
        (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
    }
    (void)emitter.Emit("]\n");
#endif
}

void AArch64AsmEmitter::EmitLazyLoadStatic(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    /* adrp xd, :got:__staticDecoupleValueOffset$$xxx+offset
     * ldr wd, [xd, #:got_lo12:__staticDecoupleValueOffset$$xxx+offset]
     * ldr wzr, [xd]
     */
    const InsnDesc *md = &AArch64CG::kMd[MOP_lazy_ldr_static];

    Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
    Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
    const OpndDesc *prop0 = md->GetOpndDes(0);
    A64OpndEmitVisitor visitor(emitter, prop0);
    auto *stImmOpnd = static_cast<StImmOperand *>(opnd1);
    CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Emitter::EmitLazyLoadStatic");

    /* emit "adrp xd, :got:__staticDecoupleValueOffset$$xxx+offset" */
    (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
    opnd0->Accept(visitor);
    (void)emitter.Emit(", ");
    (void)emitter.Emit(stImmOpnd->GetName());
    if (stImmOpnd->GetOffset() != 0) {
        (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
    }
    (void)emitter.Emit("\t// lazy load static.\n");

    /* emit "ldr wd, [xd, #:got_lo12:__staticDecoupleValueOffset$$xxx+offset]" */
    (void)emitter.Emit("\tldr\t");
    static_cast<RegOperand *>(opnd0)->SetRefField(true);
#ifdef USE_32BIT_REF
    const OpndDesc prop2(prop0->GetOperandType(), prop0->GetRegProp(), prop0->GetSize() / 2);
    opnd0->Emit(emitter, &prop2); /* ldr wd, ... for terminal system */
#else
    opnd0->Accept(visitor); /* ldr xd, ... for qemu */
#endif /* USE_32BIT_REF */
    static_cast<RegOperand *>(opnd0)->SetRefField(false);
    (void)emitter.Emit(", ");
    (void)emitter.Emit("[");
    opnd0->Accept(visitor);
    (void)emitter.Emit(",");
    (void)emitter.Emit("#");
    (void)emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
    if (stImmOpnd->GetOffset() != 0) {
        (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
    }
    (void)emitter.Emit("]\t// lazy load static.\n");

    /* emit "ldr wzr, [xd]" */
    (void)emitter.Emit("\t").Emit("ldr\twzr, [");
    opnd0->Accept(visitor);
    (void)emitter.Emit("]\t// lazy load static.\n");
#endif
}

void AArch64AsmEmitter::EmitArrayClassCacheLoad(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    /* adrp xd, :got:__arrayClassCacheTable$$xxx+offset
     * ldr wd, [xd, #:got_lo12:__arrayClassCacheTable$$xxx+offset]
     * ldr wzr, [xd]
     */
    const InsnDesc *md = &AArch64CG::kMd[MOP_arrayclass_cache_ldr];
    Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
    Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
    const OpndDesc *prop0 = md->GetOpndDes(kInsnFirstOpnd);
    A64OpndEmitVisitor visitor(emitter, prop0);
    auto *stImmOpnd = static_cast<StImmOperand *>(opnd1);
    CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Emitter::EmitLazyLoadStatic");

    /* emit "adrp xd, :got:__arrayClassCacheTable$$xxx+offset" */
    (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
    opnd0->Accept(visitor);
    (void)emitter.Emit(", ");
    (void)emitter.Emit(stImmOpnd->GetName());
    if (stImmOpnd->GetOffset() != 0) {
        (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
    }
    (void)emitter.Emit("\t// load array class.\n");

    /* emit "ldr wd, [xd, #:got_lo12:__arrayClassCacheTable$$xxx+offset]" */
    (void)emitter.Emit("\tldr\t");
    static_cast<RegOperand *>(opnd0)->SetRefField(true);
#ifdef USE_32BIT_REF
    const OpndDesc prop2(prop0->GetOperandType(), prop0->GetRegProp(), prop0->GetSize() / 2);
    A64OpndEmitVisitor visitor2(emitter, prop2);
    opnd0->Accept(visitor2); /* ldr wd, ... for terminal system */
#else
    opnd0->Accept(visitor); /* ldr xd, ... for qemu */
#endif /* USE_32BIT_REF */
    static_cast<RegOperand *>(opnd0)->SetRefField(false);
    (void)emitter.Emit(", ");
    (void)emitter.Emit("[");
    opnd0->Accept(visitor);
    (void)emitter.Emit(",");
    (void)emitter.Emit("#");
    (void)emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
    if (stImmOpnd->GetOffset() != 0) {
        (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
    }
    (void)emitter.Emit("]\t// load array class.\n");

    /* emit "ldr wzr, [xd]" */
    (void)emitter.Emit("\t").Emit("ldr\twzr, [");
    opnd0->Accept(visitor);
    (void)emitter.Emit("]\t// check resolve array class.\n");
#endif
}

/*
 * intrinsic_get_add_int w0, xt, wt, ws, x1, x2, w3, label
 * add    xt, x1, x2
 * label:
 * ldaxr  w0, [xt]
 * add    wt, w0, w3
 * stlxr  ws, wt, [xt]
 * cbnz   ws, label
 */
void AArch64AsmEmitter::EmitGetAndAddInt(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    DEBUG_ASSERT(insn.GetOperandSize() > kInsnEighthOpnd, "ensure the oprands number");
    (void)emitter.Emit("\t//\tstart of Unsafe.getAndAddInt.\n");
    Operand *tempOpnd0 = &insn.GetOperand(kInsnSecondOpnd);
    Operand *tempOpnd1 = &insn.GetOperand(kInsnThirdOpnd);
    Operand *tempOpnd2 = &insn.GetOperand(kInsnFourthOpnd);
    Operand *objOpnd = &insn.GetOperand(kInsnFifthOpnd);
    Operand *offsetOpnd = &insn.GetOperand(kInsnSixthOpnd);
    Operand *deltaOpnd = &insn.GetOperand(kInsnSeventhOpnd);
    Operand *labelOpnd = &insn.GetOperand(kInsnEighthOpnd);
    A64OpndEmitVisitor visitor(emitter, nullptr);
    /* emit add. */
    (void)emitter.Emit("\t").Emit("add").Emit("\t");
    tempOpnd0->Accept(visitor);
    (void)emitter.Emit(", ");
    objOpnd->Accept(visitor);
    (void)emitter.Emit(", ");
    offsetOpnd->Accept(visitor);
    (void)emitter.Emit("\n");
    /* emit label. */
    labelOpnd->Accept(visitor);
    (void)emitter.Emit(":\n");
    Operand *retVal = &insn.GetOperand(kInsnFirstOpnd);
    const MOperator mOp = insn.GetMachineOpcode();
    const InsnDesc *md = &AArch64CG::kMd[mOp];
    const OpndDesc *retProp = md->opndMD[kInsnFirstOpnd];
    A64OpndEmitVisitor retVisitor(emitter, retProp);
    /* emit ldaxr */
    (void)emitter.Emit("\t").Emit("ldaxr").Emit("\t");
    retVal->Accept(retVisitor);
    (void)emitter.Emit(", [");
    tempOpnd0->Accept(visitor);
    (void)emitter.Emit("]\n");
    /* emit add. */
    (void)emitter.Emit("\t").Emit("add").Emit("\t");
    tempOpnd1->Accept(retVisitor);
    (void)emitter.Emit(", ");
    retVal->Accept(retVisitor);
    (void)emitter.Emit(", ");
    deltaOpnd->Accept(retVisitor);
    (void)emitter.Emit("\n");
    /* emit stlxr. */
    (void)emitter.Emit("\t").Emit("stlxr").Emit("\t");
    tempOpnd2->Accept(visitor);
    (void)emitter.Emit(", ");
    tempOpnd1->Accept(retVisitor);
    (void)emitter.Emit(", [");
    tempOpnd0->Accept(visitor);
    (void)emitter.Emit("]\n");
    /* emit cbnz. */
    (void)emitter.Emit("\t").Emit("cbnz").Emit("\t");
    tempOpnd2->Accept(visitor);
    (void)emitter.Emit(", ");
    labelOpnd->Accept(visitor);
    (void)emitter.Emit("\n");
    (void)emitter.Emit("\t//\tend of Unsafe.getAndAddInt.\n");
#endif
}

/*
 * intrinsic_get_set_int w0, xt, ws, x1, x2, w3, label
 * add    xt, x1, x2
 * label:
 * ldaxr  w0, [xt]
 * stlxr  ws, w3, [xt]
 * cbnz   ws, label
 */
void AArch64AsmEmitter::EmitGetAndSetInt(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    /* MOP_get_and_setI and MOP_get_and_setL have 7 operands */
    DEBUG_ASSERT(insn.GetOperandSize() > kInsnSeventhOpnd, "ensure the operands number");
    Operand *tempOpnd0 = &insn.GetOperand(kInsnSecondOpnd);
    Operand *tempOpnd1 = &insn.GetOperand(kInsnThirdOpnd);
    Operand *objOpnd = &insn.GetOperand(kInsnFourthOpnd);
    Operand *offsetOpnd = &insn.GetOperand(kInsnFifthOpnd);
    A64OpndEmitVisitor visitor(emitter, nullptr);
    /* add    x1, x1, x2 */
    (void)emitter.Emit("\tadd\t");
    tempOpnd0->Accept(visitor);
    (void)emitter.Emit(", ");
    objOpnd->Accept(visitor);
    (void)emitter.Emit(", ");
    offsetOpnd->Accept(visitor);
    (void)emitter.Emit("\n");
    Operand *labelOpnd = &insn.GetOperand(kInsnSeventhOpnd);
    /* label: */
    labelOpnd->Accept(visitor);
    (void)emitter.Emit(":\n");
    Operand *retVal = &insn.GetOperand(kInsnFirstOpnd);
    /* ldaxr  w0, [xt] */
    (void)emitter.Emit("\tldaxr\t");
    retVal->Accept(visitor);
    (void)emitter.Emit(", [");
    tempOpnd0->Accept(visitor);
    (void)emitter.Emit("]\n");
    Operand *newValueOpnd = &insn.GetOperand(kInsnSixthOpnd);
    /* stlxr  ws, w3, [xt] */
    (void)emitter.Emit("\tstlxr\t");
    tempOpnd1->Accept(visitor);
    (void)emitter.Emit(", ");
    newValueOpnd->Accept(visitor);
    (void)emitter.Emit(", [");
    tempOpnd0->Accept(visitor);
    (void)emitter.Emit("]\n");
    /* cbnz   w2, label */
    (void)emitter.Emit("\tcbnz\t");
    tempOpnd1->Accept(visitor);
    (void)emitter.Emit(", ");
    labelOpnd->Accept(visitor);
    (void)emitter.Emit("\n");
#endif
}

/*
 * intrinsic_string_indexof w0, x1, w2, x3, w4, x5, x6, x7, x8, x9, w10,
 *                          Label.FIRST_LOOP, Label.STR2_NEXT, Label.STR1_LOOP,
 *                          Label.STR1_NEXT, Label.LAST_WORD, Label.NOMATCH, Label.RET
 * cmp       w4, w2
 * b.gt      .Label.NOMATCH
 * sub       w2, w2, w4
 * sub       w4, w4, #8
 * mov       w10, w2
 * uxtw      x4, w4
 * uxtw      x2, w2
 * add       x3, x3, x4
 * add       x1, x1, x2
 * neg       x4, x4
 * neg       x2, x2
 * ldr       x5, [x3,x4]
 * .Label.FIRST_LOOP:
 * ldr       x7, [x1,x2]
 * cmp       x5, x7
 * b.eq      .Label.STR1_LOOP
 * .Label.STR2_NEXT:
 * adds      x2, x2, #1
 * b.le      .Label.FIRST_LOOP
 * b         .Label.NOMATCH
 * .Label.STR1_LOOP:
 * adds      x8, x4, #8
 * add       x9, x2, #8
 * b.ge      .Label.LAST_WORD
 * .Label.STR1_NEXT:
 * ldr       x6, [x3,x8]
 * ldr       x7, [x1,x9]
 * cmp       x6, x7
 * b.ne      .Label.STR2_NEXT
 * adds      x8, x8, #8
 * add       x9, x9, #8
 * b.lt      .Label.STR1_NEXT
 * .Label.LAST_WORD:
 * ldr       x6, [x3]
 * sub       x9, x1, x4
 * ldr       x7, [x9,x2]
 * cmp       x6, x7
 * b.ne      .Label.STR2_NEXT
 * add       w0, w10, w2
 * b         .Label.RET
 * .Label.NOMATCH:
 * mov       w0, #-1
 * .Label.RET:
 */
void AArch64AsmEmitter::EmitStringIndexOf(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    /* MOP_string_indexof has 18 operands */
    DEBUG_ASSERT(insn.GetOperandSize() == 18, "ensure the operands number");
    Operand *patternLengthOpnd = &insn.GetOperand(kInsnFifthOpnd);
    Operand *srcLengthOpnd = &insn.GetOperand(kInsnThirdOpnd);
    const std::string patternLengthReg =
        AArch64CG::intRegNames[AArch64CG::kR64List][static_cast<RegOperand *>(patternLengthOpnd)->GetRegisterNumber()];
    const std::string srcLengthReg =
        AArch64CG::intRegNames[AArch64CG::kR64List][static_cast<RegOperand *>(srcLengthOpnd)->GetRegisterNumber()];
    A64OpndEmitVisitor visitor(emitter, nullptr);
    /* cmp       w4, w2 */
    (void)emitter.Emit("\tcmp\t");
    patternLengthOpnd->Accept(visitor);
    (void)emitter.Emit(", ");
    srcLengthOpnd->Accept(visitor);
    (void)emitter.Emit("\n");
    /* the 16th operand of MOP_string_indexof is Label.NOMATCH */
    Operand *labelNoMatch = &insn.GetOperand(16);
    /* b.gt      Label.NOMATCH */
    (void)emitter.Emit("\tb.gt\t");
    labelNoMatch->Accept(visitor);
    (void)emitter.Emit("\n");
    /* sub       w2, w2, w4 */
    (void)emitter.Emit("\tsub\t");
    srcLengthOpnd->Accept(visitor);
    (void)emitter.Emit(", ");
    srcLengthOpnd->Accept(visitor);
    (void)emitter.Emit(", ");
    patternLengthOpnd->Accept(visitor);
    (void)emitter.Emit("\n");
    /* sub       w4, w4, #8 */
    (void)emitter.Emit("\tsub\t");
    patternLengthOpnd->Accept(visitor);
    (void)emitter.Emit(", ");
    patternLengthOpnd->Accept(visitor);
    (void)emitter.Emit(", #8\n");
    /* the 10th operand of MOP_string_indexof is w10 */
    Operand *resultTmp = &insn.GetOperand(10);
    /* mov       w10, w2 */
    (void)emitter.Emit("\tmov\t");
    resultTmp->Accept(visitor);
    (void)emitter.Emit(", ");
    srcLengthOpnd->Accept(visitor);
    (void)emitter.Emit("\n");
    /* uxtw      x4, w4 */
    (void)emitter.Emit("\tuxtw\t").Emit(patternLengthReg);
    (void)emitter.Emit(", ");
    patternLengthOpnd->Accept(visitor);
    (void)emitter.Emit("\n");
    /* uxtw      x2, w2 */
    (void)emitter.Emit("\tuxtw\t").Emit(srcLengthReg);
    (void)emitter.Emit(", ");
    srcLengthOpnd->Accept(visitor);
    (void)emitter.Emit("\n");
    Operand *patternStringBaseOpnd = &insn.GetOperand(kInsnFourthOpnd);
    /* add       x3, x3, x4 */
    (void)emitter.Emit("\tadd\t");
    patternStringBaseOpnd->Accept(visitor);
    (void)emitter.Emit(", ");
    patternStringBaseOpnd->Accept(visitor);
    (void)emitter.Emit(", ").Emit(patternLengthReg);
    (void)emitter.Emit("\n");
    Operand *srcStringBaseOpnd = &insn.GetOperand(kInsnSecondOpnd);
    /* add       x1, x1, x2 */
    (void)emitter.Emit("\tadd\t");
    srcStringBaseOpnd->Accept(visitor);
    (void)emitter.Emit(", ");
    srcStringBaseOpnd->Accept(visitor);
    (void)emitter.Emit(", ").Emit(srcLengthReg);
    (void)emitter.Emit("\n");
    /* neg       x4, x4 */
    (void)emitter.Emit("\tneg\t").Emit(patternLengthReg);
    (void)emitter.Emit(", ").Emit(patternLengthReg);
    (void)emitter.Emit("\n");
    /* neg       x2, x2 */
    (void)emitter.Emit("\tneg\t").Emit(srcLengthReg);
    (void)emitter.Emit(", ").Emit(srcLengthReg);
    (void)emitter.Emit("\n");
    Operand *first = &insn.GetOperand(kInsnSixthOpnd);
    /* ldr       x5, [x3,x4] */
    (void)emitter.Emit("\tldr\t");
    first->Accept(visitor);
    (void)emitter.Emit(", [");
    patternStringBaseOpnd->Accept(visitor);
    (void)emitter.Emit(",").Emit(patternLengthReg);
    (void)emitter.Emit("]\n");
    /* the 11th operand of MOP_string_indexof is Label.FIRST_LOOP */
    Operand *labelFirstLoop = &insn.GetOperand(11);
    /* .Label.FIRST_LOOP: */
    labelFirstLoop->Accept(visitor);
    (void)emitter.Emit(":\n");
    /* the 7th operand of MOP_string_indexof is x7 */
    Operand *ch2 = &insn.GetOperand(7);
    /* ldr       x7, [x1,x2] */
    (void)emitter.Emit("\tldr\t");
    ch2->Accept(visitor);
    (void)emitter.Emit(", [");
    srcStringBaseOpnd->Accept(visitor);
    (void)emitter.Emit(",").Emit(srcLengthReg);
    (void)emitter.Emit("]\n");
    /* cmp       x5, x7 */
    (void)emitter.Emit("\tcmp\t");
    first->Accept(visitor);
    (void)emitter.Emit(", ");
    ch2->Accept(visitor);
    (void)emitter.Emit("\n");
    /* the 13th operand of MOP_string_indexof is Label.STR1_LOOP */
    Operand *labelStr1Loop = &insn.GetOperand(13);
    /* b.eq      .Label.STR1_LOOP */
    (void)emitter.Emit("\tb.eq\t");
    labelStr1Loop->Accept(visitor);
    (void)emitter.Emit("\n");
    /* the 12th operand of MOP_string_indexof is Label.STR2_NEXT */
    Operand *labelStr2Next = &insn.GetOperand(12);
    /* .Label.STR2_NEXT: */
    labelStr2Next->Accept(visitor);
    (void)emitter.Emit(":\n");
    /* adds      x2, x2, #1 */
    (void)emitter.Emit("\tadds\t").Emit(srcLengthReg);
    (void)emitter.Emit(", ").Emit(srcLengthReg);
    (void)emitter.Emit(", #1\n");
    /* b.le      .Label.FIRST_LOOP */
    (void)emitter.Emit("\tb.le\t");
    labelFirstLoop->Accept(visitor);
    (void)emitter.Emit("\n");
    /* b         .Label.NOMATCH */
    (void)emitter.Emit("\tb\t");
    labelNoMatch->Accept(visitor);
    (void)emitter.Emit("\n");
    /* .Label.STR1_LOOP: */
    labelStr1Loop->Accept(visitor);
    (void)emitter.Emit(":\n");
    /* the 8th operand of MOP_string_indexof is x8 */
    Operand *tmp1 = &insn.GetOperand(kInsnEighthOpnd);
    /* adds      x8, x4, #8 */
    (void)emitter.Emit("\tadds\t");
    tmp1->Accept(visitor);
    (void)emitter.Emit(", ").Emit(patternLengthReg);
    (void)emitter.Emit(", #8\n");
    /* the 9th operand of MOP_string_indexof is x9 */
    Operand *tmp2 = &insn.GetOperand(9);
    /* add       x9, x2, #8 */
    (void)emitter.Emit("\tadd\t");
    tmp2->Accept(visitor);
    (void)emitter.Emit(", ").Emit(srcLengthReg);
    (void)emitter.Emit(", #8\n");
    /* the 15th operand of MOP_string_indexof is Label.LAST_WORD */
    Operand *labelLastWord = &insn.GetOperand(15);
    /* b.ge      .Label.LAST_WORD */
    (void)emitter.Emit("\tb.ge\t");
    labelLastWord->Accept(visitor);
    (void)emitter.Emit("\n");
    /* the 14th operand of MOP_string_indexof is Label.STR1_NEXT */
    Operand *labelStr1Next = &insn.GetOperand(14);
    /* .Label.STR1_NEXT: */
    labelStr1Next->Accept(visitor);
    (void)emitter.Emit(":\n");
    /* the 6th operand of MOP_string_indexof is x6 */
    Operand *ch1 = &insn.GetOperand(6);
    /* ldr       x6, [x3,x8] */
    (void)emitter.Emit("\tldr\t");
    ch1->Accept(visitor);
    (void)emitter.Emit(", [");
    patternStringBaseOpnd->Accept(visitor);
    (void)emitter.Emit(",");
    tmp1->Accept(visitor);
    (void)emitter.Emit("]\n");
    /* ldr       x7, [x1,x9] */
    (void)emitter.Emit("\tldr\t");
    ch2->Accept(visitor);
    (void)emitter.Emit(", [");
    srcStringBaseOpnd->Accept(visitor);
    (void)emitter.Emit(",");
    tmp2->Accept(visitor);
    (void)emitter.Emit("]\n");
    /* cmp       x6, x7 */
    (void)emitter.Emit("\tcmp\t");
    ch1->Accept(visitor);
    (void)emitter.Emit(", ");
    ch2->Accept(visitor);
    (void)emitter.Emit("\n");
    /* b.ne      .Label.STR2_NEXT */
    (void)emitter.Emit("\tb.ne\t");
    labelStr2Next->Accept(visitor);
    (void)emitter.Emit("\n");
    /* adds      x8, x8, #8 */
    (void)emitter.Emit("\tadds\t");
    tmp1->Accept(visitor);
    (void)emitter.Emit(", ");
    tmp1->Accept(visitor);
    (void)emitter.Emit(", #8\n");
    /* add       x9, x9, #8 */
    (void)emitter.Emit("\tadd\t");
    tmp2->Accept(visitor);
    (void)emitter.Emit(", ");
    tmp2->Accept(visitor);
    (void)emitter.Emit(", #8\n");
    /* b.lt      .Label.STR1_NEXT */
    (void)emitter.Emit("\tb.lt\t");
    labelStr1Next->Accept(visitor);
    (void)emitter.Emit("\n");
    /* .Label.LAST_WORD: */
    labelLastWord->Accept(visitor);
    (void)emitter.Emit(":\n");
    /* ldr       x6, [x3] */
    (void)emitter.Emit("\tldr\t");
    ch1->Accept(visitor);
    (void)emitter.Emit(", [");
    patternStringBaseOpnd->Accept(visitor);
    (void)emitter.Emit("]\n");
    /* sub       x9, x1, x4 */
    (void)emitter.Emit("\tsub\t");
    tmp2->Accept(visitor);
    (void)emitter.Emit(", ");
    srcStringBaseOpnd->Accept(visitor);
    (void)emitter.Emit(", ").Emit(patternLengthReg);
    (void)emitter.Emit("\n");
    /* ldr       x7, [x9,x2] */
    (void)emitter.Emit("\tldr\t");
    ch2->Accept(visitor);
    (void)emitter.Emit(", [");
    tmp2->Accept(visitor);
    (void)emitter.Emit(", ").Emit(srcLengthReg);
    (void)emitter.Emit("]\n");
    /* cmp       x6, x7 */
    (void)emitter.Emit("\tcmp\t");
    ch1->Accept(visitor);
    (void)emitter.Emit(", ");
    ch2->Accept(visitor);
    (void)emitter.Emit("\n");
    /* b.ne      .Label.STR2_NEXT */
    (void)emitter.Emit("\tb.ne\t");
    labelStr2Next->Accept(visitor);
    (void)emitter.Emit("\n");
    Operand *retVal = &insn.GetOperand(kInsnFirstOpnd);
    /* add       w0, w10, w2 */
    (void)emitter.Emit("\tadd\t");
    retVal->Accept(visitor);
    (void)emitter.Emit(", ");
    resultTmp->Accept(visitor);
    (void)emitter.Emit(", ");
    srcLengthOpnd->Accept(visitor);
    (void)emitter.Emit("\n");
    /* the 17th operand of MOP_string_indexof Label.ret */
    Operand *labelRet = &insn.GetOperand(17);
    /* b         .Label.ret */
    (void)emitter.Emit("\tb\t");
    labelRet->Accept(visitor);
    (void)emitter.Emit("\n");
    /* .Label.NOMATCH: */
    labelNoMatch->Accept(visitor);
    (void)emitter.Emit(":\n");
    /* mov       w0, #-1 */
    (void)emitter.Emit("\tmov\t");
    retVal->Accept(visitor);
    (void)emitter.Emit(", #-1\n");
    /* .Label.ret: */
    labelRet->Accept(visitor);
    (void)emitter.Emit(":\n");
#endif
}

/*
 * intrinsic_compare_swap_int x0, xt, xs, x1, x2, w3, w4, lable1, label2
 * add       xt, x1, x2
 * label1:
 * ldaxr     ws, [xt]
 * cmp       ws, w3
 * b.ne      label2
 * stlxr     ws, w4, [xt]
 * cbnz      ws, label1
 * label2:
 * cset      x0, eq
 */
void AArch64AsmEmitter::EmitCompareAndSwapInt(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    /* MOP_compare_and_swapI and MOP_compare_and_swapL have 8 operands */
    DEBUG_ASSERT(insn.GetOperandSize() > kInsnEighthOpnd, "ensure the operands number");
    const MOperator mOp = insn.GetMachineOpcode();
    const InsnDesc *md = &AArch64CG::kMd[mOp];
    Operand *temp0 = &insn.GetOperand(kInsnSecondOpnd);
    Operand *temp1 = &insn.GetOperand(kInsnThirdOpnd);
    Operand *obj = &insn.GetOperand(kInsnFourthOpnd);
    Operand *offset = &insn.GetOperand(kInsnFifthOpnd);
    A64OpndEmitVisitor visitor(emitter, nullptr);
    /* add       xt, x1, x2 */
    (void)emitter.Emit("\tadd\t");
    temp0->Accept(visitor);
    (void)emitter.Emit(", ");
    obj->Accept(visitor);
    (void)emitter.Emit(", ");
    offset->Accept(visitor);
    (void)emitter.Emit("\n");
    Operand *label1 = &insn.GetOperand(kInsnEighthOpnd);
    /* label1: */
    label1->Accept(visitor);
    (void)emitter.Emit(":\n");
    /* ldaxr     ws, [xt] */
    (void)emitter.Emit("\tldaxr\t");
    temp1->Accept(visitor);
    (void)emitter.Emit(", [");
    temp0->Accept(visitor);
    (void)emitter.Emit("]\n");
    Operand *expectedValue = &insn.GetOperand(kInsnSixthOpnd);
    const OpndDesc *expectedValueProp = md->opndMD[kInsnSixthOpnd];
    /* cmp       ws, w3 */
    (void)emitter.Emit("\tcmp\t");
    temp1->Accept(visitor);
    (void)emitter.Emit(", ");
    A64OpndEmitVisitor visitorExpect(emitter, expectedValueProp);
    expectedValue->Accept(visitorExpect);
    (void)emitter.Emit("\n");
    constexpr uint32 kInsnNinethOpnd = 8;
    Operand *label2 = &insn.GetOperand(kInsnNinethOpnd);
    /* b.ne      label2 */
    (void)emitter.Emit("\tbne\t");
    label2->Accept(visitor);
    (void)emitter.Emit("\n");
    Operand *newValue = &insn.GetOperand(kInsnSeventhOpnd);
    /* stlxr     ws, w4, [xt] */
    (void)emitter.Emit("\tstlxr\t");
    (void)emitter.Emit(
        AArch64CG::intRegNames[AArch64CG::kR32List][static_cast<RegOperand *>(temp1)->GetRegisterNumber()]);
    (void)emitter.Emit(", ");
    newValue->Accept(visitor);
    (void)emitter.Emit(", [");
    temp0->Accept(visitor);
    (void)emitter.Emit("]\n");
    /* cbnz      ws, label1 */
    (void)emitter.Emit("\tcbnz\t");
    (void)emitter.Emit(
        AArch64CG::intRegNames[AArch64CG::kR32List][static_cast<RegOperand *>(temp1)->GetRegisterNumber()]);
    (void)emitter.Emit(", ");
    label1->Accept(visitor);
    (void)emitter.Emit("\n");
    /* label2: */
    label2->Accept(visitor);
    (void)emitter.Emit(":\n");
    Operand *retVal = &insn.GetOperand(kInsnFirstOpnd);
    /* cset      x0, eq */
    (void)emitter.Emit("\tcset\t");
    retVal->Accept(visitor);
    (void)emitter.Emit(", EQ\n");
#endif
}

void AArch64AsmEmitter::EmitCTlsDescRel(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    const InsnDesc *md = &AArch64CG::kMd[MOP_tls_desc_rel];
    Operand *result = &insn.GetOperand(kInsnFirstOpnd);
    Operand *src = &insn.GetOperand(kInsnSecondOpnd);
    Operand *symbol = &insn.GetOperand(kInsnThirdOpnd);
    auto stImmOpnd = static_cast<StImmOperand *>(symbol);
    A64OpndEmitVisitor resultVisitor(emitter, md->opndMD[0]);
    A64OpndEmitVisitor srcVisitor(emitter, md->opndMD[1]);
    (void)emitter.Emit("\t").Emit("add").Emit("\t");
    result->Accept(resultVisitor);
    (void)emitter.Emit(", ");
    src->Accept(srcVisitor);
    (void)emitter.Emit(", #:tprel_hi12:").Emit(stImmOpnd->GetName()).Emit(", lsl #12\n");
    (void)emitter.Emit("\t").Emit("add").Emit("\t");
    result->Accept(resultVisitor);
    (void)emitter.Emit(", ");
    result->Accept(resultVisitor);
    (void)emitter.Emit(", #:tprel_lo12_nc:").Emit(stImmOpnd->GetName()).Emit("\n");
#endif
}

void AArch64AsmEmitter::EmitCTlsDescCall(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    const InsnDesc *md = &AArch64CG::kMd[MOP_tls_desc_call];
    Operand *func = &insn.GetOperand(kInsnFirstOpnd);
    Operand *symbol = &insn.GetOperand(kInsnThirdOpnd);
    const OpndDesc *prop = md->opndMD[0];
    auto *stImmOpnd = static_cast<StImmOperand *>(symbol);
    const std::string &symName = stImmOpnd->GetName();
    A64OpndEmitVisitor funcVisitor(emitter, prop);
    /*  adrp    x0, :tlsdesc:symbol */
    (void)emitter.Emit("\t").Emit("adrp\tx0, :tlsdesc:").Emit(symName).Emit("\n");
    /*  ldr x1, [x0, #tlsdesc_lo12:symbol] */
    (void)emitter.Emit("\t").Emit("ldr").Emit("\t");
    func->Accept(funcVisitor);
    (void)emitter.Emit(", [x0, #:tlsdesc_lo12:").Emit(symName).Emit("]\n");
    /*  add x0 ,#tlsdesc_lo12:symbol */
    (void)emitter.Emit("\t").Emit("add\tx0, x0, :tlsdesc_lo12:").Emit(symName).Emit("\n");
    /* .tlsdesccall <symbolName> */
    (void)emitter.Emit("\t").Emit(".tlsdesccall").Emit("\t").Emit(symName).Emit("\n");
    /* blr xd */
    (void)emitter.Emit("\t").Emit("blr").Emit("\t");
    func->Accept(funcVisitor);
    (void)emitter.Emit("\n");
#endif
}

void AArch64AsmEmitter::EmitSyncLockTestSet(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    const InsnDesc *md = &AArch64CG::kMd[insn.GetMachineOpcode()];
    auto *result = &insn.GetOperand(kInsnFirstOpnd);
    auto *temp = &insn.GetOperand(kInsnSecondOpnd);
    auto *addr = &insn.GetOperand(kInsnThirdOpnd);
    auto *value = &insn.GetOperand(kInsnFourthOpnd);
    auto *label = &insn.GetOperand(kInsnFifthOpnd);
    A64OpndEmitVisitor resultVisitor(emitter, md->opndMD[kInsnFirstOpnd]);
    A64OpndEmitVisitor tempVisitor(emitter, md->opndMD[kInsnSecondOpnd]);
    A64OpndEmitVisitor addrVisitor(emitter, md->opndMD[kInsnThirdOpnd]);
    A64OpndEmitVisitor valueVisitor(emitter, md->opndMD[kInsnFourthOpnd]);
    A64OpndEmitVisitor labelVisitor(emitter, md->opndMD[kInsnFifthOpnd]);
    /* label: */
    label->Accept(labelVisitor);
    (void)emitter.Emit(":\n");
    /* ldxr x0, [x2] */
    (void)emitter.Emit("\t").Emit("ldxr").Emit("\t");
    result->Accept(resultVisitor);
    (void)emitter.Emit(", [");
    addr->Accept(addrVisitor);
    (void)emitter.Emit("]\n");
    /* stxr w1, x3, [x2] */
    (void)emitter.Emit("\t").Emit("stxr").Emit("\t");
    temp->Accept(tempVisitor);
    (void)emitter.Emit(", ");
    value->Accept(valueVisitor);
    (void)emitter.Emit(", [");
    addr->Accept(addrVisitor);
    (void)emitter.Emit("]\n");
    /* cbnz w1, label */
    (void)emitter.Emit("\t").Emit("cbnz").Emit("\t");
    temp->Accept(tempVisitor);
    (void)emitter.Emit(", ");
    label->Accept(labelVisitor);
    (void)emitter.Emit("\n");
    /* dmb ish */
    (void)emitter.Emit("\t").Emit("dmb").Emit("\t").Emit("ish").Emit("\n");
#endif
}

void AArch64AsmEmitter::EmitPureCall(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    const InsnDesc *md = &AArch64CG::kMd[insn.GetMachineOpcode()];
    auto *callee = &insn.GetOperand(kInsnFirstOpnd);
    A64OpndEmitVisitor calleeVisitor(emitter, md->opndMD[kInsnFirstOpnd]);
    (void)emitter.Emit("\t").Emit("blr").Emit("\t");
    callee->Accept(calleeVisitor);
    (void)emitter.Emit("\n");
#endif
}

void AArch64AsmEmitter::EmitCheckThrowPendingException(Emitter &emitter, Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    /*
     * mrs x16, TPIDR_EL0
     * ldr x16, [x16, #64]
     * ldr x16, [x16, #8]
     * cbz x16, .lnoexception
     * bl MCC_ThrowPendingException
     * .lnoexception:
     */
    (void)emitter.Emit("\t").Emit("mrs").Emit("\tx16, TPIDR_EL0");
    (void)emitter.Emit("\n");
    (void)emitter.Emit("\t").Emit("ldr").Emit("\tx16, [x16, #64]");
    (void)emitter.Emit("\n");
    (void)emitter.Emit("\t").Emit("ldr").Emit("\tx16, [x16, #8]");
    (void)emitter.Emit("\n");
    (void)emitter.Emit("\t").Emit("cbz").Emit("\tx16, .lnoeh.").Emit(maplebe::CG::GetCurCGFunc()->GetName());
    (void)emitter.Emit("\n");
    (void)emitter.Emit("\t").Emit("bl").Emit("\tMCC_ThrowPendingException");
    (void)emitter.Emit("\n");
    (void)emitter.Emit(".lnoeh.").Emit(maplebe::CG::GetCurCGFunc()->GetName()).Emit(":");
    (void)emitter.Emit("\n");
#endif
}

void AArch64AsmEmitter::EmitLazyBindingRoutine(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    /* ldr xzr, [xs] */
    const InsnDesc *md = &AArch64CG::kMd[MOP_adrp_ldr];

    Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
    const OpndDesc *prop0 = md->opndMD[0];
    A64OpndEmitVisitor visitor(emitter, prop0);

    /* emit "ldr  xzr,[xs]" */
#ifdef USE_32BIT_REF
    (void)emitter.Emit("\t").Emit("ldr").Emit("\twzr, [");
#else
    (void)emitter.Emit("\t").Emit("ldr").Emit("\txzr, [");
#endif /* USE_32BIT_REF */
    opnd0->Accept(visitor);
    (void)emitter.Emit("]");
    (void)emitter.Emit("\t// Lazy binding\n");
#endif
}

#ifdef ARK_LITECG_DEBUG
struct CfiDescr {
    const std::string name;
    uint32 opndCount;
    /* create 3 OperandType array to store cfi instruction's operand type */
    std::array<Operand::OperandType, 3> opndTypes;
};

static CfiDescr cfiDescrTable[cfi::kOpCfiLast + 1] = {
#define CFI_DEFINE(k, sub, n, o0, o1, o2) {".cfi_" #k, n, {Operand::kOpd##o0, Operand::kOpd##o1, Operand::kOpd##o2}},
#define ARM_DIRECTIVES_DEFINE(k, sub, n, o0, o1, o2) \
    {"." #k, n, {Operand::kOpd##o0, Operand::kOpd##o1, Operand::kOpd##o2}},
#include "cfi.def"
#undef CFI_DEFINE
#undef ARM_DIRECTIVES_DEFINE
    {".cfi_undef", 0, {Operand::kOpdUndef, Operand::kOpdUndef, Operand::kOpdUndef}}};
#endif

void AArch64AsmEmitter::EmitAArch64CfiInsn(Emitter &emitter, const Insn &insn) const
{
#ifdef ARK_LITECG_DEBUG
    MOperator mOp = insn.GetMachineOpcode();
    CHECK_FATAL(mOp <= cfi::kOpCfiLast, "check overflow");
    CfiDescr &cfiDescr = cfiDescrTable[mOp];
    (void)emitter.Emit("\t").Emit(cfiDescr.name);
    for (uint32 i = 0; i < cfiDescr.opndCount; ++i) {
        (void)emitter.Emit(" ");
        Operand &curOperand = insn.GetOperand(i);
        cfi::CFIOpndEmitVisitor cfiOpndEmitVisitor(emitter);
        curOperand.Accept(cfiOpndEmitVisitor);
        if (i < (cfiDescr.opndCount - 1)) {
            (void)emitter.Emit(",");
        }
    }
    (void)emitter.Emit("\n");
#endif
}

bool AArch64AsmEmitter::CheckInsnRefField(const Insn &insn, size_t opndIndex) const
{
#ifdef ARK_LITECG_DEBUG
    if (insn.IsAccessRefField() && insn.AccessMem()) {
        Operand &opnd0 = insn.GetOperand(opndIndex);
        if (opnd0.IsRegister()) {
            static_cast<RegOperand &>(opnd0).SetRefField(true);
            return true;
        }
    }
#endif
    return false;
}
} /* namespace maplebe */
