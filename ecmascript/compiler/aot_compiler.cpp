/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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


#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/ohos/ohos_pkg_verifier.h"
#include "ecmascript/platform/aot_crash_info.h"


namespace panda::ecmascript::kungfu {
namespace {
/**
 * @param ErrCode return code of ark_aot_compiler
 * @attention it must sync with RetStatusOfCompiler of
 *            "ets_runtime/compiler_service/include/aot_compiler_constants.h"
 */
enum ErrCode {
    ERR_OK = (0),   // IMPORTANT: Only if aot compiler SUCCESS and save an/ai SUCCESS, return ERR_OK.
    ERR_FAIL = (-1),
    ERR_HELP = (1),
    ERR_NO_AP = (2),
    ERR_MERGE_AP = (3),
    ERR_CHECK_VERSION = (4),
    ERR_AN_EMPTY = (5),
    ERR_AN_FAIL = (6),
    ERR_AI_FAIL = (7),
};

bool CheckVersion(JSRuntimeOptions& runtimeOptions, AotCompilerStats& compilerStats, bool isPgoMerged)
{
    if (runtimeOptions.IsCheckPgoVersion()) {
        if (!isPgoMerged) {
            LOG_COMPILER(ERROR) << "CheckVersion ap and abc may not match";
            compilerStats.SetPgoFileLegal(false);
        }
        if (runtimeOptions.IsTargetCompilerMode()) {
            compilerStats.PrintCompilerStatsLog();
        }
        return true;
    }
    return false;
}

} // namespace

int Main(const int argc, const char **argv)
{
    if (argc < 2) { // 2: at least have two arguments
        LOG_COMPILER(ERROR) << AotCompilerPreprocessor::GetHelper();
        return ERR_FAIL;
    }

    JSRuntimeOptions runtimeOptions;
    bool retOpt = runtimeOptions.ParseCommand(argc, argv);
    if (!retOpt) {
        LOG_COMPILER(ERROR) << AotCompilerPreprocessor::GetHelper();
        return ERR_HELP;
    }

    bool ret = true;
    // ark_aot_compiler running need disable asm interpreter to disable the loading of AOT files.
    runtimeOptions.SetEnableAsmInterpreter(false);
    runtimeOptions.SetOptionsForTargetCompilation();
    EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
    if (vm == nullptr) {
        LOG_COMPILER(ERROR) << "Cannot Create vm";
        return ERR_FAIL;
    }

    {
        AOTCompilationEnv aotCompilationEnv(vm);
        ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
        LocalScope scope(vm);
        arg_list_t pandaFileNames {};
        std::map<std::string, std::shared_ptr<OhosPkgArgs>> pkgArgsMap;
        CompilationOptions cOptions(runtimeOptions);

        CompilerLog log(cOptions.logOption_);
        log.SetEnableCompilerLogTime(cOptions.compilerLogTime_);
        AotMethodLogList logList(cOptions.logMethodsList_);
        PGOProfilerDecoder profilerDecoder;

        AotCompilerPreprocessor cPreprocessor(vm, runtimeOptions, pkgArgsMap, profilerDecoder, pandaFileNames);
        if (!cPreprocessor.HandleTargetCompilerMode(cOptions) || !cPreprocessor.HandlePandaFileNames(argc, argv)) {
            return ERR_HELP;
        }

        // Need to verify package information to prevent abnormal input of information
        if (!ohos::OhosPkgVerifier::VerifyPkgInfo(cPreprocessor, cOptions)) {
            LOG_COMPILER(ERROR) << "hap verify wrong";
            return ERR_FAIL;
        }

        if (runtimeOptions.IsPartialCompilerMode() && cOptions.profilerIn_.empty()) {
            // no need to compile in partial mode without any ap files.
            return ERR_NO_AP;
        }

        AotCompilerStats compilerStats;
        std::string bundleName = "";
        if (cPreprocessor.GetMainPkgArgs()) {
            bundleName = cPreprocessor.GetMainPkgArgs()->GetBundleName();
        }
        compilerStats.SetBundleName(bundleName);
        compilerStats.SetAotFilePath(cOptions.outputFileName_);
        compilerStats.SetPgoPath(cOptions.profilerIn_);
        compilerStats.StartCompiler();
        profilerDecoder.SetHotnessThreshold(cOptions.hotnessThreshold_);
        profilerDecoder.SetInPath(cOptions.profilerIn_);
        cPreprocessor.AOTInitialize();
        std::unordered_map<CString, uint32_t> fileNameToChecksumMap;
        cPreprocessor.GenerateAbcFileInfos(fileNameToChecksumMap);

        if (runtimeOptions.IsTargetCompilerMode() && (cPreprocessor.HasExistsAOTFiles(cOptions) ||
            cPreprocessor.HasPreloadAotFile())) {
            LOG_COMPILER(ERROR) << "The AOT file already exists and will not be compiled anymore";
            return ERR_OK;
        }
        ret = cPreprocessor.GetCompilerResult();
        // Notice: lx move load pandaFileHead and verify before GeneralAbcFileInfos.
        // need support multiple abc
        auto isPgoMerged = cPreprocessor.HandleMergedPgoFile(fileNameToChecksumMap);
        if (CheckVersion(runtimeOptions, compilerStats, isPgoMerged)) {
            return ERR_CHECK_VERSION;
        }
        std::string appSignature = cPreprocessor.GetMainPkgArgsAppSignature();
        if (!isPgoMerged) {
            AOTFileGenerator::SaveEmptyAOTFile(
                cOptions.outputFileName_ + AOTFileManager::FILE_EXTENSION_AN, appSignature, true);
            AOTFileGenerator::SaveEmptyAOTFile(
                cOptions.outputFileName_ + AOTFileManager::FILE_EXTENSION_AI, appSignature, false);
            return ERR_MERGE_AP;
        }
        cPreprocessor.Process(cOptions);

        PassOptions::Builder optionsBuilder;
        PassOptions passOptions =
            optionsBuilder.EnableArrayBoundsCheckElimination(cOptions.isEnableArrayBoundsCheckElimination_)
                .EnableTypeLowering(cOptions.isEnableTypeLowering_)
                .EnableEarlyElimination(cOptions.isEnableEarlyElimination_)
                .EnableLaterElimination(cOptions.isEnableLaterElimination_)
                .EnableValueNumbering(cOptions.isEnableValueNumbering_)
                .EnableOptInlining(cOptions.isEnableOptInlining_)
                .EnableOptString(cOptions.isEnableOptString_)
                .EnableOptPGOType(cOptions.isEnableOptPGOType_)
                .EnableOptTrackField(cOptions.isEnableOptTrackField_)
                .EnableOptLoopPeeling(cOptions.isEnableOptLoopPeeling_)
                .EnableOptLoopInvariantCodeMotion(cOptions.isEnableOptLoopInvariantCodeMotion_)
                .EnableOptConstantFolding(cOptions.isEnableOptConstantFolding_)
                .EnableLazyDeopt(cOptions.isEnableLazyDeopt_)
                .EnableLexenvSpecialization(cOptions.isEnableLexenvSpecialization_)
                .EnableInlineNative(cOptions.isEnableNativeInline_)
                .EnableLoweringBuiltin(cOptions.isEnableLoweringBuiltin_)
                .EnableOptBranchProfiling(cOptions.isEnableOptBranchProfiling_)
                .EnableEscapeAnalysis(cOptions.isEnableEscapeAnalysis_)
                .EnableInductionVariableAnalysis(cOptions.isEnableInductionVariableAnalysis_)
                .EnableVerifierPass(cOptions.isEnableVerifierPass_)
                .EnableMergePoly(cOptions.isEnableMergePoly_)
                .Build();

        PassManager passManager(&aotCompilationEnv,
                                cOptions.triple_,
                                cOptions.optLevel_,
                                cOptions.relocMode_,
                                &log,
                                &logList,
                                cOptions.maxAotMethodSize_,
                                cOptions.maxMethodsInModule_,
                                profilerDecoder,
                                &passOptions,
                                cPreprocessor.GetCallMethodFlagMap(),
                                cPreprocessor.GetAbcFileInfo(),
                                cPreprocessor.GetBcInfoCollectors(),
                                cOptions.optBCRange_);

        bool isEnableLiteCG = runtimeOptions.IsCompilerEnableLiteCG();
        compilerStats.SetIsLiteCg(isEnableLiteCG);

        AOTFileGenerator generator(&log, &logList, &aotCompilationEnv, cOptions.triple_, isEnableLiteCG,
                                   cOptions.anFileMaxByteSize_);
        if (runtimeOptions.IsTargetCompilerMode() && runtimeOptions.IsEnableAotCodeComment()) {
            if (!generator.CreateAOTCodeCommentFile(cOptions.outputFileName_ + AOTFileManager::FILE_EXTENSION_AN)) {
                LOG_COMPILER(ERROR) << "Generate aot code comment file failed.";
            }
        }
        passManager.CompileValidFiles(generator, ret, compilerStats);
        if (compilerStats.GetCompilerMethodCount() == 0) {
            return runtimeOptions.IsPartialCompilerMode() ? ERR_AN_EMPTY : ERR_OK;
        }
        if (!generator.SaveAOTFile(cOptions.outputFileName_ + AOTFileManager::FILE_EXTENSION_AN, appSignature,
                                   fileNameToChecksumMap)) {
            return ERR_AN_FAIL;
        }
        if (!generator.SaveSnapshotFile()) {
            return ERR_AI_FAIL;
        }
        log.Print();
        if (runtimeOptions.IsTargetCompilerMode()) {
            compilerStats.PrintCompilerStatsLog();
        }
    }

    if (vm->GetJSOptions().IsEnableCompilerLogSnapshot()) {
        vm->PrintAOTSnapShotStats();
    }
    LOG_COMPILER(INFO) << (ret ? "ts aot compile success" : "ts aot compile failed");
    JSNApi::DestroyJSVM(vm);
    return ret ? ERR_OK : ERR_FAIL;
}
} // namespace panda::ecmascript::kungfu

int main(const int argc, const char **argv)
{
    auto result = panda::ecmascript::kungfu::Main(argc, argv);
    return result;
}
