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

#include "ecmascript/compiler/aot_compilation_env.h"
#include "ecmascript/compiler/aot_compiler_stats.h"
#include "ecmascript/compiler/aot_file/tests/aot_file_generator_mock.h"
#include "gtest/gtest.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/platform/filesystem.h"
#include "ecmascript/compiler/aot_file/an_file_data_manager.h"
#include "ecmascript/ohos/ohos_pkg_args.h"
#include "ecmascript/compiler/pass_options.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/compiler/aot_compiler_preprocessor.h"
#include "ecmascript/compiler/aot_compilation_env.h"
#include "ecmascript/ohos/ohos_pkg_verifier.h"
#include "ecmascript/js_runtime_options.h"

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
        this->testDir_.clear();
        this->testAbcPath_.clear();
        this->testRecordName_.clear();
        this->testApPath_.clear();
        this->testAnPath_.clear();
        this->testAiPath_.clear();
        this->testAotPath_.clear();
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

    void SetEnvrionmentForTest(const std::string &abcFile)
    {
        SetTestAbcPath(abcFile);
        SetRecordName(abcFile);
        SetApPath();
        SetAotPath();
        SetAnPath();
        SetAiPath();
    }

    // Get .ap pgo profile for provided abc file
    void GetApFileInTest()
    {
        JSRuntimeOptions runtimeOptions;
        runtimeOptions.SetEnablePGOProfiler(true);
        runtimeOptions.SetPGOProfilerPath(testApPath_);
        runtimeOptions.SetEntryPoint(testRecordName_);
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
        ASSERT_TRUE(vm != nullptr);
        bool isMergeAbc = runtimeOptions.GetMergeAbc();
        JSNApi::SetBundle(vm, !isMergeAbc);
        bool res = JSNApi::Execute(vm, testAbcPath_, runtimeOptions.GetEntryPoint());
        ASSERT_TRUE(res);
        JSNApi::DestroyJSVM(vm);
        ASSERT_TRUE(Exists(testApPath_));
    }

    void GetAnFileInTest(size_t limitSizeByte = 0)
    {
        bool ret = true;
        JSRuntimeOptions runtimeOptions;
        runtimeOptions.SetPGOProfilerPath(testApPath_);
        runtimeOptions.SetAOTOutputFile(testAotPath_);
        runtimeOptions.SetEnableAsmInterpreter(false);
        runtimeOptions.SetOptionsForTargetCompilation();
        ecmascript::AnFileDataManager::GetInstance()->SetEnable(true);
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);

        ASSERT_TRUE(vm != nullptr);
        {
            AOTCompilationEnv aotCompilationEnv(vm);
            ThreadManagedScope managedScope(vm->GetJSThread());
            LocalScope scope(vm);
            arg_list_t pandaFileNames {testAbcPath_};
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

    void SetRecordName(const std::string &abcFile)
    {
        testRecordName_ = abcFile.substr(0, abcFile.find_last_of("."));
        ASSERT_TRUE(!testRecordName_.empty());
    }

    std::string testDir_;
    std::string testAbcPath_;
    std::string testRecordName_;
    std::string testApPath_;
    std::string testAnPath_;
    std::string testAiPath_;
    std::string testAotPath_;
    size_t testAnExpectedSize_ = 0;
};

HWTEST_F_L0(AOTFileTest, fileSizeEqualExpectedTest)
{
    // This case use to test the compiler out .an file size equal to expected size by calcutelate.
    // Test file use file_size_test.abc
    std::string testFile = "file_size_test.abc";
    SetEnvrionmentForTest(testFile);
    GetApFileInTest();
    GetAnFileInTest();
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
    GetApFileInTest();
    GetAnFileInTest(1);
    ASSERT_FALSE(Exists(testAnPath_));
    ASSERT_FALSE(Exists(testAiPath_));
    // We can still calcutelate the correct size
    ASSERT_NE(testAnExpectedSize_, 0);
}
}  // namespace panda::test
