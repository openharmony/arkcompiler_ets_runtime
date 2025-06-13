/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"

#include "ecmascript/compiler/aot_compilation_env.h"
#include "ecmascript/compiler/aot_compiler_preprocessor.h"
#include "ecmascript/compiler/aot_compiler_stats.h"
#include "ecmascript/compiler/aot_file/an_file_data_manager.h"
#include "ecmascript/compiler/aot_file/tests/aot_file_generator_mock.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/compiler/pass_options.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/ohos/ohos_pkg_args.h"
#include "ecmascript/platform/filesystem.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::kungfu;
using namespace panda::ecmascript::pgo;
using namespace panda::panda_file;
using namespace panda::ecmascript::filesystem;

namespace panda::test {
class AOTFileTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownTestCase";
    }

    void SetUp() override
    {
        this->testDir_ = std::string(TARGET_ABC_PATH);
    }

    void TearDown() override
    {
        if (Exists(this->testAnPath_.c_str())) {
            Unlink(this->testAnPath_.c_str());
        }
        if (Exists(this->testApPath_.c_str())) {
            Unlink(this->testApPath_.c_str());
        }
        if (Exists(this->testAiPath_.c_str())) {
            Unlink(this->testAiPath_.c_str());
        }
        if (Exists(this->workerAnPath_.c_str())) {
            Unlink(this->workerAnPath_.c_str());
        }
        if (Exists(this->workerApPath_.c_str())) {
            Unlink(this->workerApPath_.c_str());
        }
        if (Exists(this->workerAiPath_.c_str())) {
            Unlink(this->workerAiPath_.c_str());
        }
        this->testDir_.clear();
        this->testAbcPath_.clear();
        this->testRecordName_.clear();
        this->testApPath_.clear();
        this->testAnPath_.clear();
        this->testAiPath_.clear();
        this->testAotPath_.clear();
        this->workerAbcPath_.clear();
        this->workerRecordName_.clear();
        this->workerApPath_.clear();
        this->workerAnPath_.clear();
        this->workerAiPath_.clear();
        this->workerAotPath_.clear();
        this->testAnExpectedSize_ = 0;
    }

protected:
    void SetTestAbcPath(const std::string &abcFile)
    {
        testAbcPath_ = testDir_ + abcFile;
        ASSERT_TRUE(Exists(testAbcPath_));
    }

    void SetApPath()
    {
        testApPath_ = testDir_ + testRecordName_ + ".ap";
    }

    void SetAotPath()
    {
        testAotPath_ = testDir_ + testRecordName_;
    }

    void SetAnPath()
    {
        testAnPath_ = testDir_ + testRecordName_ + ".an";
    }

    void SetAiPath()
    {
        testAiPath_ = testDir_ + testRecordName_ + ".ai";
    }

    void SetRecordName(const std::string &abcFile)
    {
        testRecordName_ = abcFile.substr(0, abcFile.find_last_of("."));
        ASSERT_TRUE(!testRecordName_.empty());
    }

    void SetEnvrionmentForTest(const std::string &abcFile)
    {
        SetTestAbcPath(abcFile);
        SetRecordName(abcFile);
        SetApPath();
        SetAotPath();
        SetAnPath();
        SetAiPath();
    }

    void SetWorkerAbcPath(const std::string &abcFile)
    {
        workerAbcPath_ = testDir_ + abcFile;
        ASSERT_TRUE(Exists(workerAbcPath_));
    }

    void SetWorkerApPath()
    {
        workerApPath_ = testDir_ + workerRecordName_ + ".ap";
    }

    void SetWorkerAotPath()
    {
        workerAotPath_ = testDir_ + workerRecordName_;
    }

    void SetWorkerAnPath()
    {
        workerAnPath_ = testDir_ + workerRecordName_ + ".an";
    }

    void SetWorkerAiPath()
    {
        workerAiPath_ = testDir_ + workerRecordName_ + ".ai";
    }

    void SetWorkerRecordName(const std::string &abcFile)
    {
        workerRecordName_ = abcFile.substr(0, abcFile.find_last_of("."));
        ASSERT_TRUE(!workerRecordName_.empty());
    }

    void SetEnvrionmentForWorker(const std::string &abcFile)
    {
        SetWorkerAbcPath(abcFile);
        SetWorkerRecordName(abcFile);
        SetWorkerApPath();
        SetWorkerAotPath();
        SetWorkerAnPath();
        SetWorkerAiPath();
    }

    // Get .ap pgo profile for provided abc file
    void GetApFileInTest(std::string testApPath, std::string testRecordName, std::string testAbcPath)
    {
        JSRuntimeOptions runtimeOptions;
        runtimeOptions.SetEnablePGOProfiler(true);
        runtimeOptions.SetPGOProfilerPath(testApPath);
        runtimeOptions.SetEntryPoint(testRecordName);
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
        ASSERT_TRUE(vm != nullptr);
        bool isMergeAbc = runtimeOptions.GetMergeAbc();
        JSNApi::SetBundle(vm, !isMergeAbc);
        bool res = JSNApi::Execute(vm, testAbcPath, runtimeOptions.GetEntryPoint());
        ASSERT_TRUE(res);
        JSNApi::DestroyJSVM(vm);
        ASSERT_TRUE(Exists(testApPath));
    }

    void GetAnFileInTest(std::string testApPath, std::string testAotPath, std::string testAbcPath,
                         size_t limitSizeByte = 0)
    {
        bool ret = true;
        JSRuntimeOptions runtimeOptions;
        runtimeOptions.SetPGOProfilerPath(testApPath);
        runtimeOptions.SetAOTOutputFile(testAotPath);
        runtimeOptions.SetEnableAsmInterpreter(false);
        runtimeOptions.SetOptionsForTargetCompilation();
        ecmascript::AnFileDataManager::GetInstance()->SetEnable(true);
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);

        ASSERT_TRUE(vm != nullptr);
        {
            AOTCompilationEnv aotCompilationEnv(vm);
            ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
            LocalScope scope(vm);
            arg_list_t pandaFileNames {testAbcPath};
            std::map<std::string, std::shared_ptr<OhosPkgArgs>> pkgArgsMap;
            CompilationOptions cOptions(runtimeOptions);
            CompilerLog log(cOptions.logOption_);
            AotMethodLogList logList(cOptions.logMethodsList_);
            PGOProfilerDecoder profilerDecoder;
            AotCompilerPreprocessor cPreprocessor(vm, runtimeOptions, pkgArgsMap, profilerDecoder, pandaFileNames);
            AotCompilerStats compilerStats;
            compilerStats.SetAotFilePath(cOptions.outputFileName_);
            compilerStats.SetPgoPath(cOptions.profilerIn_);
            compilerStats.StartCompiler();
            profilerDecoder.SetHotnessThreshold(cOptions.hotnessThreshold_);
            profilerDecoder.SetInPath(cOptions.profilerIn_);
            cPreprocessor.AOTInitialize();
            std::unordered_map<CString, uint32_t> originFilenameToChecksumMap;
            cPreprocessor.GenerateAbcFileInfos(originFilenameToChecksumMap);
            ret = cPreprocessor.GetCompilerResult();
            cPreprocessor.HandleMergedPgoFile(originFilenameToChecksumMap);
            cPreprocessor.Process(cOptions);
            PassOptions::Builder optionsBuilder;
            PassOptions passOptions = optionsBuilder.Build();
            PassManager passManager(&aotCompilationEnv, cOptions.triple_, cOptions.optLevel_, cOptions.relocMode_, &log,
                                    &logList, cOptions.maxAotMethodSize_, cOptions.maxMethodsInModule_, profilerDecoder,
                                    &passOptions, cPreprocessor.GetCallMethodFlagMap(), cPreprocessor.GetAbcFileInfo(),
                                    cPreprocessor.GetBcInfoCollectors(), cOptions.optBCRange_);
            AOTFileGeneratorMock generator(&log, &logList, &aotCompilationEnv, cOptions.triple_, false, limitSizeByte);
            passManager.CompileValidFiles(generator, ret, compilerStats);
            if (generator.SaveAndGetAOTFileSize(cOptions.outputFileName_ + AOTFileManager::FILE_EXTENSION_AN, "",
                                                testAnExpectedSize_, originFilenameToChecksumMap)) {
                generator.SaveSnapshotFile();
            }
        }
        JSNApi::DestroyJSVM(vm);
        GTEST_LOG_(INFO) << "testAnPath_: " << testAnPath_;
    }

    void LoadAiInMultiThreadTest()
    {
        JSRuntimeOptions runtimeOptions;
        runtimeOptions.SetEntryPoint(testRecordName_);
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
        std::string moduleName = "file_size_test";
        std::string anFileName = "file_size_test.an";
        std::string workerModuleName = "file_size_worker";
        std::string workerAnFileName = "file_size_worker.an";
        // init aot env
        ecmascript::AnFileDataManager::GetInstance()->SetDir(testDir_);
        ecmascript::AnFileDataManager::GetInstance()->SetEnable(true);
        {
            // simulate the case load aot file in main thread for entry-hap
            vm->SetModuleName(moduleName.c_str());
            JSNApi::LoadAotFile(vm, moduleName);
            uint32_t anIndex = ecmascript::AnFileDataManager::GetInstance()->SafeGetFileInfoIndex(anFileName);
            ASSERT_EQ(anIndex, 0);
            bool res = JSNApi::Execute(vm, testAbcPath_, runtimeOptions.GetEntryPoint());
            ASSERT_TRUE(res);
            ASSERT_EQ(vm->GetAOTFileManager()->GetPandaFiles(0).size(), 1);
        }
        {
            // simulate the case load aot file in main thread for feature-hap
            vm->SetModuleName(workerModuleName.c_str());  // make sure the module name is the same with hap name
            JSNApi::LoadAotFile(vm, workerModuleName);
            uint32_t workerAnIndex =
                ecmascript::AnFileDataManager::GetInstance()->SafeGetFileInfoIndex(workerAnFileName);
            ASSERT_EQ(workerAnIndex, 1);
            bool res = JSNApi::Execute(vm, workerAbcPath_, workerRecordName_);
            ASSERT_TRUE(res);
            ASSERT_EQ(vm->GetAOTFileManager()->GetPandaFiles(1).size(), 1);
        }
        std::thread t1([&]() {
            JSRuntimeOptions runtimeOptions;
            runtimeOptions.SetEntryPoint(workerRecordName_);
            EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
            ASSERT_TRUE(vm != nullptr);
            vm->SetModuleName(moduleName.c_str());  // assume module name for worker wont be right all the time
            ASSERT_EQ(vm->GetAOTFileManager()->GetPandaFiles(1).size(), 0);
            bool res = JSNApi::Execute(vm, workerAbcPath_, runtimeOptions.GetEntryPoint());
            ASSERT_TRUE(res);
            ASSERT_EQ(vm->GetAOTFileManager()->GetPandaFiles(1).size(), 1);
            JSNApi::DestroyJSVM(vm);
        });
        {
            ecmascript::ThreadSuspensionScope suspensionScope(vm->GetJSThread());
            t1.join();
        }

        JSNApi::DestroyJSVM(vm);
    }

    std::string testDir_ {""};
    std::string testAbcPath_ {""};
    std::string testRecordName_ {""};
    std::string testApPath_ {""};
    std::string testAnPath_ {""};
    std::string testAiPath_ {""};
    std::string testAotPath_ {""};
    std::string workerAbcPath_ {""};
    std::string workerRecordName_ {""};
    std::string workerApPath_ {""};
    std::string workerAnPath_ {""};
    std::string workerAiPath_ {""};
    std::string workerAotPath_ {""};
    size_t testAnExpectedSize_ {0};
};

HWTEST_F_L0(AOTFileTest, fileSizeEqualExpectedTest)
{
    // This case use to test the compiler out .an file size equal to expected size by calculate.
    // Test file use file_size_test.abc
    std::string testFile = "file_size_test.abc";
    SetEnvrionmentForTest(testFile);
    GetApFileInTest(testApPath_, testRecordName_, testAbcPath_);
    GetAnFileInTest(testApPath_, testAotPath_, testAbcPath_);
    ASSERT_TRUE(Exists(testAnPath_));
    ASSERT_TRUE(Exists(testAiPath_));
    ASSERT_EQ(testAnExpectedSize_, FileSize(testAnPath_));
}

HWTEST_F_L0(AOTFileTest, dontSaveAnFile)
{
    // This case use to test the compiler out .an file size less than provided limit
    // Then generator will not save .an and .ai file
    // Test file use file_size_test.abc
    std::string testFile = "file_size_test.abc";
    SetEnvrionmentForTest(testFile);
    GetApFileInTest(testApPath_, testRecordName_, testAbcPath_);
    GetAnFileInTest(testApPath_, testAotPath_, testAbcPath_, 1);
    ASSERT_FALSE(Exists(testAnPath_));
    ASSERT_FALSE(Exists(testAiPath_));
    // We can still calculate the correct size
    ASSERT_NE(testAnExpectedSize_, 0);
}

HWTEST_F_L0(AOTFileTest, loadAiInMultiThread)
{
    std::string testFile = "file_size_test.abc";
    SetEnvrionmentForTest(testFile);
    GetApFileInTest(testApPath_, testRecordName_, testAbcPath_);
    GetAnFileInTest(testApPath_, testAotPath_, testAbcPath_);
    ASSERT_TRUE(Exists(testAnPath_));
    ASSERT_TRUE(Exists(testAiPath_));

    std::string workerFile = "file_size_worker.abc";
    SetEnvrionmentForWorker(workerFile);
    GetApFileInTest(workerApPath_, workerRecordName_, workerAbcPath_);
    GetAnFileInTest(workerApPath_, workerAotPath_, workerAbcPath_);
    ASSERT_TRUE(Exists(workerAnPath_));
    ASSERT_TRUE(Exists(workerAiPath_));

    LoadAiInMultiThreadTest();
}
}  // namespace panda::test
