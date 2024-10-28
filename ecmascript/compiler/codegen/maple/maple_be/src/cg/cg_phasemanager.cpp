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

#include "cg_phasemanager.h"
#include <vector>
#include <string>
#include "cg_option.h"
#include "args.h"
#include "label_creation.h"
#include "isel.h"
#include "offset_adjust.h"
#include "emit.h"
#include "reg_alloc.h"
#if TARGAARCH64
#include "aarch64_emitter.h"
#include "aarch64_obj_emitter.h"
#include "aarch64_cg.h"
#endif
#if defined(TARGRISCV64)
#include "riscv64_emitter.h"
#endif
#if defined(TARGX86_64)
#include "x64_cg.h"
#include "x64_emitter.h"
#include "string_utils.h"
#endif
#include "triple.h"

namespace maplebe {
#define CLANG (module.GetSrcLang() == kSrcLangC)

#define RELEASE(pointer)            \
    do {                            \
        if ((pointer) != nullptr) { \
            delete (pointer);       \
            (pointer) = nullptr;    \
        }                           \
    } while (0)

namespace {

#ifdef ARK_LITECG_DEBUG
void DumpMIRFunc(MIRFunction &func, const char *msg, bool printAlways = false, const char *extraMsg = nullptr)
{
    bool dumpAll = (CGOptions::GetDumpPhases().find("*") != CGOptions::GetDumpPhases().end());
    bool dumpFunc = CGOptions::FuncFilter(func.GetName());

    if (printAlways || (dumpAll && dumpFunc)) {
        LogInfo::MapleLogger() << msg << '\n';
        func.Dump();

        if (extraMsg) {
            LogInfo::MapleLogger() << extraMsg << '\n';
        }
    }
}
#endif

} /* anonymous namespace */

void CgFuncPM::GenerateOutPutFile(MIRModule &m)
{
    CHECK_FATAL(cg != nullptr, "cg is null");
    CHECK_FATAL(Triple::GetTriple().GetArch() != Triple::ArchType::UnknownArch, "should have triple init before!");
    auto codegen = cg;
    if (Triple::GetTriple().GetArch() == Triple::ArchType::x64) {
        cg->Emit([codegen, &m](Emitter *emitter) {
            assembler::Assembler &objAssm = static_cast<X64Emitter &>(*emitter).GetAssembler();
            if (!codegen->GetCGOptions().SuppressFileInfo()) {
                objAssm.InitialFileInfo(m.GetInputFileName());
            }
        });
    } else if (Triple::GetTriple().GetArch() == Triple::ArchType::aarch64) {
        cg->template Emit<CG::EmitterType::AsmEmitter>([codegen, &m](Emitter *emitter) {
            if (!codegen->GetCGOptions().SuppressFileInfo()) {
                emitter->EmitFileInfo(m.GetInputFileName());
            }
        });
    } else {
        CHECK_FATAL(false, "unsupportted target!");
    }
}

bool CgFuncPM::FuncLevelRun(CGFunc &cgFunc, AnalysisDataManager &serialADM)
{
    bool changed = false;
    for (size_t i = 0; i < phasesSequence.size(); ++i) {
        SolveSkipFrom(CGOptions::GetSkipFromPhase(), i);
        const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
        if (curPhase->IsAnalysis()) {
            changed |= RunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(*curPhase, serialADM, cgFunc);
        } else {
            changed |= RunTransformPhase<MapleFunctionPhase<CGFunc>, CGFunc>(*curPhase, serialADM, cgFunc);
            DumpFuncCGIR(cgFunc, curPhase->PhaseName());
        }
        SolveSkipAfter(CGOptions::GetSkipAfterPhase(), i);
    }
    return changed;
}

void CgFuncPM::PostOutPut(MIRModule &m)
{
    if (Triple::GetTriple().GetArch() == Triple::ArchType::x64) {
        cg->Emit([this](Emitter * emitter) {
            X64Emitter *x64Emitter = static_cast<X64Emitter *>(emitter);
            assembler::Assembler &assm = x64Emitter->GetAssembler();
            x64Emitter->EmitGlobalVariable(*cg);
            assm.FinalizeFileInfo();
            assm.CloseOutput();
        });
    } else if (Triple::GetTriple().GetArch() == Triple::ArchType::aarch64) {
        cg->template Emit<CG::EmitterType::AsmEmitter>([this, &m](Emitter* emitter) {
            emitter->EmitHugeSoRoutines(true);
            /* Emit global info */
            EmitGlobalInfo(m);
        });
        cg->template Emit<CG::EmitterType::ObjEmiter>([](Emitter* emitter) {
            emitter->Finish();
            emitter->CloseOutput();
        });
    } else {
        CHECK_FATAL(false, "unsupported target");
    }
}

/* =================== new phase manager ===================  */
#ifdef RA_PERF_ANALYSIS
extern void printLSRATime();
extern void printRATime();
#endif

bool CgFuncPM::PhaseRun(MIRModule &m)
{
    // registry target based on build, cgfunc, emitter need to be registried.
    InitializeAllTargetInfos(m.GetMemPool());
    std::string compileTarget = "";
    if (Triple::GetTriple().IsAarch64BeOrLe()) {
        compileTarget = "aarch64";
    } else if (Triple::GetTriple().GetArch() == Triple::ArchType::x64) {
        compileTarget = "x86";
    } else {
        CHECK_FATAL(false, "unsupport");
    }
    const Target *TheTarget = nullptr;
    TheTarget = TargetRegistry::lookupTarget(compileTarget);
    // get target based on option
    CreateCGAndBeCommon(m, TheTarget);
    bool changed = false;
    if (cgOptions->IsRunCG()) {
        GenerateOutPutFile(m);

        /* Run the cg optimizations phases */
        PrepareLower(m);

        uint32 countFuncId = 0;
        unsigned long rangeNum = 0;

        auto userDefinedOptLevel = cgOptions->GetOptimizeLevel();
        cg->EnrollTargetPhases(this);

        auto admMempool = AllocateMemPoolInPhaseManager("cg phase manager's analysis data manager mempool");
        auto *serialADM = GetManagerMemPool()->New<AnalysisDataManager>(*(admMempool.get()));
        for (auto it = m.GetFunctionList().begin(); it != m.GetFunctionList().end(); ++it) {
            DEBUG_ASSERT(serialADM->CheckAnalysisInfoEmpty(), "clean adm before function run");
            MIRFunction *mirFunc = *it;
            if (mirFunc->GetBody() == nullptr) {
                continue;
            }
            if (userDefinedOptLevel == CGOptions::kLevel2 && m.HasPartO2List()) {
#ifdef ARK_LITECG_DEBUG
                if (m.IsInPartO2List(mirFunc->GetNameStrIdx())) {
                    cgOptions->EnableO2();
                } else {
                    cgOptions->EnableO0();
                }
#endif
                ClearAllPhases();
                cg->EnrollTargetPhases(this);
                cg->UpdateCGOptions(*cgOptions);
                Globals::GetInstance()->SetOptimLevel(cgOptions->GetOptimizeLevel());
            }
            /* LowerIR. */
            m.SetCurFunction(mirFunc);

            if (m.GetFlavor() != MIRFlavor::kFlavorLmbc) {
                DoFuncCGLower(m, *mirFunc);
            }
            /* create CGFunc */
            MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(mirFunc->GetStIdx().Idx());
            CHECK_NULL_FATAL(funcSt);
            auto funcMp = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, funcSt->GetName());
            auto stackMp = std::make_unique<StackMemPool>(funcMp->GetCtrler(), "");
            MapleAllocator funcScopeAllocator(funcMp.get());
            mirFunc->SetPuidxOrigin(++countFuncId);
            CGFunc *cgFunc =
                cg->CreateCGFunc(m, *mirFunc, *beCommon, *funcMp, *stackMp, funcScopeAllocator, countFuncId);
            CHECK_FATAL(cgFunc != nullptr, "Create CG Function failed in cg_phase_manager");
            CG::SetCurCGFunc(*cgFunc);

            /* Run the cg optimizations phases. */
            if (CGOptions::UseRange() && rangeNum >= CGOptions::GetRangeBegin() &&
                rangeNum <= CGOptions::GetRangeEnd()) {
                CGOptions::EnableInRange();
            }
            changed = FuncLevelRun(*cgFunc, *serialADM);
            /* Delete mempool. */
            mirFunc->ReleaseCodeMemory();
            ++rangeNum;
            CGOptions::DisableInRange();
        }
        PostOutPut(m);
#ifdef RA_PERF_ANALYSIS
        if (cgOptions->IsEnableTimePhases()) {
            printLSRATime();
            printRATime();
        }
#endif
    } else {
        LogInfo::MapleLogger(kLlErr) << "Skipped generating .s because -no-cg is given" << '\n';
    }
    RELEASE(cg);
    RELEASE(beCommon);
    return changed;
}

void CgFuncPM::DumpFuncCGIR(const CGFunc &f, const std::string &phaseName) const
{
#ifdef ARK_LITECG_DEBUG
    if (CGOptions::DumpPhase(phaseName) && CGOptions::FuncFilter(f.GetName())) {
        LogInfo::MapleLogger() << "\n******** CG IR After " << phaseName << ": *********\n";
        f.DumpCGIR();
    }
#endif
}

void CgFuncPM::EmitGlobalInfo(MIRModule &m) const
{
    EmitDuplicatedAsmFunc(m);
    cg->template Emit<CG::EmitterType::AsmEmitter>([](Emitter* emitter) {
        emitter->EmitGlobalVariable();
        emitter->CloseOutput();
        emitter->WriteDebugCommentToFile();
    });
}

void CgFuncPM::CreateCGAndBeCommon(MIRModule &m, const Target *t)
{
    DEBUG_ASSERT(cgOptions != nullptr, "New cg phase manager running FAILED  :: cgOptions unset");
    auto outputFileName = m.GetOutputFileName();
    Emitter *objEmitter = nullptr;
    Emitter *asmEmitter = nullptr;
    cg = t->createCG(m, *cgOptions, cgOptions->GetEHExclusiveFunctionNameVec(), CGOptions::GetCyclePatternMap());
    CHECK_FATAL(cg, "you may not register the target");
    if (Triple::GetTriple().GetArch() == Triple::ArchType::x64) {
        assembler::Assembler *asmAssembler = nullptr;
        if (cg->GetCGOptions().IsAsmEmitterEnable()) {
            asmAssembler = new assembler::AsmAssembler(outputFileName);
            asmEmitter = t->createDecoupledEmitter(*cg, *asmAssembler);
        }
        outputFileName = outputFileName.replace(outputFileName.length() - 1, 1, 1, 'o');
        auto objAssembler = new assembler::ElfAssembler(outputFileName);
        objEmitter = t->createDecoupledEmitter(*cg, *objAssembler);
    } else if (Triple::GetTriple().GetArch() == Triple::ArchType::aarch64) {
        if (cgOptions->IsAsmEmitterEnable()) {
            asmEmitter = m.GetMemPool()->New<AArch64AsmEmitter>(*cg, outputFileName);
        }
        outputFileName = outputFileName.replace(outputFileName.length() - 1, 1, 1, 'o');
        objEmitter = m.GetMemPool()->New<AArch64ObjEmitter>(*cg, outputFileName);
    } else {
        CHECK_FATAL(false, "unsupportted");
    }
    CHECK_FATAL(objEmitter, "you may not register emitter");
    cg->SetObjEmitter(*objEmitter);
    if (cg->GetCGOptions().IsAsmEmitterEnable()) {
        CHECK_FATAL(asmEmitter, "you may not register emitter");
        cg->SetAsmEmitter(*asmEmitter);
    }
#ifdef ARK_LITECG_DEBUG
    TargetMachine *targetMachine = t->createTargetMachine();
    CHECK_FATAL(targetMachine, "you may not register targetMachine");
    cg->SetTargetMachine(*targetMachine);
#endif

    /* We initialize a couple of BECommon's tables using the size information of GlobalTables.type_table_.
     * So, BECommon must be allocated after all the parsing is done and user-defined types are all acounted.
     */
    beCommon = new BECommon(m);
    Globals::GetInstance()->SetBECommon(*beCommon);
    Globals::GetInstance()->SetTarget(*cg);

    if (Triple::GetTriple().IsAarch64BeOrLe()) {
        CGOptions::EnableFramePointer();
    }
}

void CgFuncPM::PrepareLower(MIRModule &m)
{
    mirLower = GetManagerMemPool()->New<MIRLower>(m, nullptr);
    mirLower->Init();
    cgLower =
        GetManagerMemPool()->New<CGLowerer>(m, *beCommon, cg->GenerateExceptionHandlingCode(), cg->GenerateVerboseCG());
    cgLower->SetCheckLoadStore(false);
}

void CgFuncPM::DoFuncCGLower(const MIRModule &m, MIRFunction &mirFunc)
{
    if (m.GetFlavor() <= kFeProduced) {
        mirLower->SetLowerCG();
        mirLower->SetMirFunc(&mirFunc);
#ifdef ARK_LITECG_DEBUG
        DumpMIRFunc(mirFunc, "************* before MIRLowerer **************");
#endif
        mirLower->LowerFunc(mirFunc);
    }

#ifdef ARK_LITECG_DEBUG
    bool isNotQuiet = false;
    DumpMIRFunc(mirFunc, "************* before CGLowerer **************", isNotQuiet);
#endif

    cgLower->LowerFunc(mirFunc);

#ifdef ARK_LITECG_DEBUG
    DumpMIRFunc(mirFunc, "************* after  CGLowerer **************", isNotQuiet,
                "************* end    CGLowerer **************");
#endif
}

void CgFuncPM::EmitDuplicatedAsmFunc(MIRModule &m) const
{
#ifdef ARK_LITECG_DEBUG
    if (CGOptions::IsDuplicateAsmFileEmpty()) {
        return;
    }

    std::ifstream duplicateAsmFileFD(CGOptions::GetDuplicateAsmFile());

    if (!duplicateAsmFileFD.is_open()) {
        duplicateAsmFileFD.close();
        ERR(kLncErr, " %s open failed!", CGOptions::GetDuplicateAsmFile().c_str());
        return;
    }
    std::string contend;
    bool onlyForFramework = false;
    bool isFramework = IsFramework(m);

    while (getline(duplicateAsmFileFD, contend)) {
        if (!contend.compare("#Libframework_start")) {
            onlyForFramework = true;
        }

        if (!contend.compare("#Libframework_end")) {
            onlyForFramework = false;
        }

        if (onlyForFramework && !isFramework) {
            continue;
        }

        cg->Emit([&contend](Emitter *emitter) {
            emitter->Emit(contend + "\n");
        });
    }
    duplicateAsmFileFD.close();
#endif
}

bool CgFuncPM::IsFramework([[maybe_unused]] MIRModule &m) const
{
    return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgFuncPM, cgFuncPhaseManager)
/* register codegen common phases */
MAPLE_TRANSFORM_PHASE_REGISTER(CgLayoutFrame, layoutstackframe)
MAPLE_TRANSFORM_PHASE_REGISTER(CgCreateLabel, createstartendlabel)
MAPLE_TRANSFORM_PHASE_REGISTER(InstructionSelector, instructionselector)
MAPLE_TRANSFORM_PHASE_REGISTER(CgMoveRegArgs, moveargs)
MAPLE_TRANSFORM_PHASE_REGISTER(CgRegAlloc, regalloc)
MAPLE_TRANSFORM_PHASE_REGISTER(CgFrameFinalize, framefinalize)
MAPLE_TRANSFORM_PHASE_REGISTER(CgGenProEpiLog, generateproepilog)
} /* namespace maplebe */
