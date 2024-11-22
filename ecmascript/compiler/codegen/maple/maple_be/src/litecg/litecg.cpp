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

#include "litecg.h"
#include "mir_builder.h"
#include "cg_option.h"
#include "cg.h"
#include "maple_phase_support.h"
#include "maple_phase.h"
#include "cg_phasemanager.h"
#include "triple.h"
#include "driver_options.h"

namespace maple {

namespace litecg {

using namespace maplebe;

LiteCG::LiteCG(Module &mirModule, const std::vector<std::string> &litecgOptions) : module(mirModule)
{
    // Create CGOption: set up default options
    // should we make CGOptions local?
    cgOptions = &CGOptions::GetInstance();
    if (!litecgOptions.empty()) {
        maplecl::CommandLine::GetCommandLine().Parse(litecgOptions, cgCategory);
        cgOptions->SolveOptions();
    }
    cgOptions->EnableLiteCG();
    cgOptions->SetOption(CGOptions::kDoCg);
    Triple::GetTriple().Init(module.IsAArch64());
    // module information prepare
    std::string moduleName = module.GetFileName();
    GStrIdx fileNameStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(moduleName);

    // is this strictly required?
    GStrIdx nameStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("INFO_filename");
    module.PushFileInfoPair(MIRInfoPair(nameStrIdx, fileNameStrIdx.GetIdx()));
    module.PushFileInfoIsString(true);

    module.SetFlavor(kFlavorUnknown);  // need a new flavor
    module.GetImportFiles().clear();

    // Setup output file name
    module.SetOutputFileName(moduleName + ".s");
}

#ifdef ARK_LITECG_DEBUG
void LiteCG::DumpIRToFile(const std::string &fileName)
{
    module.DumpToFile(fileName);
}
#endif

void LiteCG::DumpCGIR()
{
    cgOptions->GetDumpPhases().insert("*");
    cgOptions->FuncFilter("*");
}

LiteCG &LiteCG::SetupLiteCGEmitMemoryManager(
    void *codeSpace, MemoryManagerAllocateDataSectionCallback dataSectionAllocator,
    MemoryManagerSaveFunc2AddressInfoCallback funcAddressSaver,
    maplebe::MemoryManagerSaveFunc2FPtoPrevSPDeltaCallback funcFpSPDeltaSaver,
    maplebe::MemoryManagerSaveFunc2CalleeOffsetInfoCallback funcCallOffsetSaver,
    maplebe::MemoryManagerSavePC2DeoptInfoCallback pc2DeoptInfoSaver,
    maplebe::MemoryManagerSavePC2CallSiteInfoCallback pc2CallSiteInfoSaver)
{
    cgOptions->SetupEmitMemoryManager(codeSpace, dataSectionAllocator, funcAddressSaver, funcFpSPDeltaSaver,
                                      funcCallOffsetSaver, pc2DeoptInfoSaver, pc2CallSiteInfoSaver);
    return *this;
}

void LiteCG::SetAotCodeCommentFile(const std::string &aotCodeCommentFile)
{
    cgOptions->SetEmitAotCodeCommentFile(aotCodeCommentFile);
}

void LiteCG::DoCG(bool isJit)
{
    bool timePhases = cgOptions->IsEnableTimePhases();
    MPLTimer timer;
    if (timePhases) {
        timer.Start();
    }

    cgOptions->SetUseJitCodeSign(isJit);

    Globals::GetInstance()->SetOptimLevel(cgOptions->GetOptimizeLevel());

    // not sure how to do this.
    auto cgPhaseManager = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "cg function phasemanager");
    const MaplePhaseInfo *cgPMInfo = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(&CgFuncPM::id);
    auto *cgfuncPhaseManager = static_cast<CgFuncPM *>(cgPMInfo->GetConstructor()(cgPhaseManager.get()));

    if (timePhases) {
        cgfuncPhaseManager->InitTimeHandler();
    }

    /* It is a specifc work around  (need refactor) */
    cgfuncPhaseManager->SetQuiet(true);
    cgfuncPhaseManager->SetCGOptions(cgOptions);
    (void)cgfuncPhaseManager->PhaseRun(module);

    if (timePhases) {
        cgfuncPhaseManager->DumpPhaseTime();
        timer.Stop();
        LogInfo::MapleLogger() << "Mplcg consumed " << timer.ElapsedMilliseconds() << "ms" << '\n';
    }
    cgOptions->SetUseJitCodeSign(false);
}

}  // namespace litecg

}  // namespace maple
