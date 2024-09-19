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

#include "aarch64_cg.h"
#include <cinttypes>
#include "aarch64_mop_split.h"
#include "aarch64_mop_valid.h"
#include "mir_builder.h"
#include "becommon.h"
#include "label_creation.h"
#include "operand.h"

namespace maplebe {
#define DEFINE_MOP(...) {__VA_ARGS__},
const InsnDesc AArch64CG::kMd[kMopLast] = {
#include "abstract_mmir.def"
#include "aarch64_md.def"
};
#undef DEFINE_MOP

std::array<std::array<const std::string, kAllRegNum>, kIntRegTypeNum> AArch64CG::intRegNames = {
    std::array<const std::string, kAllRegNum> {
        "err",   "err0",  "err1",  "err2",  "err3",  "err4",         "err5",  "err6",  "err7",
        "err8",  "err9",  "err10", "err11", "err12", "err13",        "err14", "err15", "err16",
        "err17", "err18", "err19", "err20", "err21", "err22",        "err23", "err24", "err25",
        "err26", "err27", "err28", "err",   "err",   "err",          "errsp", "errzr", /* x29 is fp */
        "b0",    "b1",    "b2",    "b3",    "b4",    "b5",           "b6",    "b7",    "b8",
        "b9",    "b10",   "b11",   "b12",   "b13",   "b14",          "b15",   "b16",   "b17",
        "b18",   "b19",   "b20",   "b21",   "b22",   "b23",          "b24",   "b25",   "b26",
        "b27",   "b28",   "b29",   "b30",   "b31",   "errMaxRegNum", "rflag"},
    std::array<const std::string, kAllRegNum> {
        "err",   "err0",  "err1",  "err2",  "err3",  "err4",         "err5",  "err6",  "err7",
        "err8",  "err9",  "err10", "err11", "err12", "err13",        "err14", "err15", "err16",
        "err17", "err18", "err19", "err20", "err21", "err22",        "err23", "err24", "err25",
        "err26", "err27", "err28", "err29", "err30", "err31",        "errsp", "errzr", /* x29 is fp */
        "h0",    "h1",    "h2",    "h3",    "h4",    "h5",           "h6",    "h7",    "h8",
        "h9",    "h10",   "h11",   "h12",   "h13",   "h14",          "h15",   "h16",   "h17",
        "h18",   "h19",   "h20",   "h21",   "h22",   "h23",          "h24",   "h25",   "h26",
        "h27",   "h28",   "h29",   "h30",   "h31",   "errMaxRegNum", "rflag"},
    std::array<const std::string, kAllRegNum> {
        "err", "w0",  "w1",  "w2",  "w3",  "w4",  "w5",  "w6",  "w7",           "w8",   "w9",  "w10",
        "w11", "w12", "w13", "w14", "w15", "w16", "w17", "w18", "w19",          "w20",  "w21", "w22",
        "w23", "w24", "w25", "w26", "w27", "w28", "w29", "err", "err",          "wsp",  "wzr", /* x29 is fp */
        "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",  "s8",           "s9",   "s10", "s11",
        "s12", "s13", "s14", "s15", "s16", "s17", "s18", "s19", "s20",          "s21",  "s22", "s23",
        "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31", "errMaxRegNum", "rflag"},
    std::array<const std::string, kAllRegNum> {
        "err",  "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",  "x8",  "x9",
        "x10",  "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19", "x20",
        "x21",  "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x29" /* use X40 when debug */,
        "sp",   "xzr", /* x29 is fp */
        "d0",   "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",  "d8",  "d9",  "d10",
        "d11",  "d12", "d13", "d14", "d15", "d16", "d17", "d18", "d19", "d20", "d21",
        "d22",  "d23", "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31", "errMaxRegNum",
        "rflag"},
    std::array<const std::string, kAllRegNum> {
        "err",  "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",  "x8",  "x9",
        "x10",  "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19", "x20",
        "x21",  "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x29" /* use X40 when debug */,
        "sp",   "xzr", /* x29 is fp */
        "q0",   "q1",  "q2",  "q3",  "q4",  "q5",  "q6",  "q7",  "q8",  "q9",  "q10",
        "q11",  "q12", "q13", "q14", "q15", "q16", "q17", "q18", "q19", "q20", "q21",
        "q22",  "q23", "q24", "q25", "q26", "q27", "q28", "q29", "q30", "q31", "errMaxRegNum",
        "rflag"}};

std::array<const std::string, kAllRegNum> AArch64CG::vectorRegNames = {
    "err", "err0", "err1", "err2", "err3", "err4", "err5", "err6", "err7", "err8", "err9", "err10", "err11", "err12",
    "err13", "err14", "err15", "err16", "err17", "err18", "err19", "err20", "err21", "err22",
    /* x29 is fp, err40 is fp before RA */
    "err23", "err24", "err25", "err26", "err27", "err28", "err29", "err30", "errsp", "errzr", "err40", "v0", "v1", "v2",
    "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19",
    "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31", "errMaxRegNum", "rflag"};

bool AArch64CG::IsExclusiveFunc(MIRFunction &mirFunc)
{
    const std::string &funcName = mirFunc.GetName();
    for (const auto &it : ehExclusiveNameVec) {
        if (it.compare(funcName) == 0) {
            return true;
        }
    }
    return false;
}

bool AArch64CG::IsTargetInsn(MOperator mOp) const
{
    return (mOp > MOP_undef && mOp <= MOP_nop);
}
bool AArch64CG::IsClinitInsn(MOperator mOp) const
{
    return (mOp == MOP_clinit || mOp == MOP_clinit_tail || mOp == MOP_adrp_ldr);
}
bool AArch64CG::IsPseudoInsn(MOperator mOp) const
{
    return (mOp >= MOP_pseudo_param_def_x && mOp < MOP_nop);
}

bool AArch64CG::IsEffectiveCopy(Insn &insn) const
{
    MOperator mOp = insn.GetMachineOpcode();
    if (mOp >= MOP_xmovrr && mOp <= MOP_xvmovrv) {
        return true;
    }
    if ((mOp >= MOP_xaddrrr && mOp <= MOP_ssub) || (mOp >= MOP_xlslrri6 && mOp <= MOP_wlsrrrr)) {
        Operand &opnd2 = insn.GetOperand(kInsnThirdOpnd);
        if (opnd2.IsIntImmediate()) {
            auto &immOpnd = static_cast<ImmOperand &>(opnd2);
            if (immOpnd.IsZero()) {
                return true;
            }
        }
    }
    if (mOp > MOP_xmulrrr && mOp <= MOP_xvmuld) {
        Operand &opnd2 = insn.GetOperand(kInsnThirdOpnd);
        if (opnd2.IsIntImmediate()) {
            auto &immOpnd = static_cast<ImmOperand &>(opnd2);
            if (immOpnd.GetValue() == 1) {
                return true;
            }
        }
    }
    return false;
}

void AArch64CG::DumpTargetOperand(Operand &opnd, const OpndDesc &opndDesc) const
{
    A64OpndDumpVisitor visitor(opndDesc);
    opnd.Accept(visitor);
}

void AArch64CG::EmitGCTIBLabel(GCTIBKey *key, const std::string &gcTIBName,
                               std::vector<uint64> &bitmapWords, uint32 rcHeader)
{
    GCTIBPattern *ptn = memPool->New<GCTIBPattern>(*key, *memPool);
    (void)keyPatternMap.insert(std::make_pair(key, ptn));
    (void)symbolPatternMap.insert(std::make_pair(gcTIBName, ptn));

    /* Emit GCTIB pattern */
    std::string ptnString = "\t.type " + ptn->GetName() + ", %object\n" + "\t.data\n" + "\t.align 3\n";
    MIRSymbol *gcTIBSymbol = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
        GlobalTables::GetStrTable().GetStrIdxFromName(gcTIBName));
    if (gcTIBSymbol != nullptr && gcTIBSymbol->GetStorageClass() == kScFstatic) {
        ptnString += "\t.local ";
    } else {
        ptnString += "\t.global ";
    }

    Emit([&ptnString, ptn, rcHeader, &bitmapWords](Emitter *emitter) {
        emitter->Emit(ptnString);
        emitter->Emit(ptn->GetName());
        emitter->Emit("\n");

        /* Emit the GCTIB pattern label for the class */
        emitter->Emit(ptn->GetName());
        emitter->Emit(":\n");

        emitter->Emit("\t.long ");
        emitter->EmitHexUnsigned(rcHeader);
        emitter->Emit("\n");

        /* generate n_bitmap word */
        emitter->Emit("\t.long "); /* AArch64-specific. Generate a 64-bit value. */
        emitter->EmitDecUnsigned(bitmapWords.size());
        emitter->Emit("\n");

        /* Emit each bitmap word */
        for (const auto &bitmapWord : bitmapWords) {
            emitter->Emit("\t.quad "); /* AArch64-specific. Generate a 64-bit value. */
            emitter->EmitHexUnsigned(bitmapWord);
            emitter->Emit("\n");
        }
    });
    if (gcTIBSymbol != nullptr && gcTIBSymbol->GetStorageClass() != kScFstatic) {
        /* add local symbol REF_XXX to every global GCTIB symbol */
        CreateRefSymForGlobalPtn(*ptn);
        keyPatternMap[key] = ptn;
    }
}

/*
 * Add local symbol REF_XXX to global GCTIB symbol,
 * and replace the global GCTIBPattern in keyPatternMap.
 */
void AArch64CG::CreateRefSymForGlobalPtn(GCTIBPattern &ptn) const
{
    const std::string &refPtnString = REF_PREFIX_STR + ptn.GetName();
    const std::string &ptnString = "\t.type " + refPtnString + ", %object\n" + "\t.data\n" + "\t.align 3\n" +
                                   "\t.local " + refPtnString + "\n" + refPtnString + ":\n" + "\t.quad " +
                                   ptn.GetName() + "\n";
    Emit([&ptnString](Emitter *emitter) {
        emitter->Emit(ptnString);
    });
    ptn.SetName(refPtnString);
}

#ifdef ARK_LITECG_DEBUG
std::string AArch64CG::FindGCTIBPatternName(const std::string &name) const
{
    auto iter = symbolPatternMap.find(name);
    if (iter == symbolPatternMap.end()) {
        CHECK_FATAL(false, "No GCTIB pattern found for symbol: %s", name.c_str());
    }
    return iter->second->GetName();
}
#endif

void AArch64CG::EnrollTargetPhases(MaplePhaseManager *pm) const
{
    CGOptions::DisableCGSSA();
#include "aarch64_phases.def"
}

PhiOperand &AArch64CG::CreatePhiOperand(MemPool &mp, MapleAllocator &mAllocator)
{
    return *mp.New<PhiOperand>(mAllocator);
}
} /* namespace maplebe */
