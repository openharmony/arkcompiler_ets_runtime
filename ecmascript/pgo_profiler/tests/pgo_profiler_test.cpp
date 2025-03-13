/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "gtest/gtest.h"

#include "ecmascript/base/file_header.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/elements.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_file_info.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_method_type_set.h"
#include "ecmascript/pgo_profiler/pgo_context.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_encoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/tests/pgo_context_mock.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::pgo;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
class PGOProfilerTest : public testing::Test {
public:
    using ApGenMode = PGOProfilerEncoder::ApGenMode;
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        strictMatch_ = PGOProfilerHeader::IsStrictMatch();
    }

    void TearDown() override
    {
        DestroyJSPandaFile();
        PGOProfilerManager::GetInstance()->SetDisablePGO(false);
        PGOProfilerManager::GetInstance()->Destroy();
        PGOProfilerHeader::SetStrictMatch(strictMatch_);
    }

protected:
    void CreateJSPandaFile(const CString filename, std::vector<MethodLiteral *> &methodLiterals)
    {
        std::string targetAbcPath = std::string(TARGET_ABC_PATH) + filename.c_str();
        auto pfPtr = panda_file::OpenPandaFileOrZip(targetAbcPath, panda_file::File::READ_WRITE);
        JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
        pf_ = pfManager->NewJSPandaFile(pfPtr.release(), filename);

        const File *file = pf_->GetPandaFile();
        auto classes = pf_->GetClasses();

        for (size_t i = 0; i < classes.Size(); i++) {
            panda_file::File::EntityId classId(classes[i]);
            if (!classId.IsValid() || pf_->IsExternal(classId)) {
                continue;
            }
            ClassDataAccessor cda(*file, classId);
            cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
                auto *methodLiteral = new MethodLiteral(mda.GetMethodId());
                methodLiteral->Initialize(pf_.get());
                pf_->SetMethodLiteralToMap(methodLiteral);
                methodLiterals.push_back(methodLiteral);
            });
        }
    }

    void CreateJSPandaFile(const char *source, const CString filename, std::vector<MethodLiteral *> &methodLiterals)
    {
        Parser parser;
        const std::string fn = "SRC.abc";  // test file name : "SRC.abc"
        auto res = parser.Parse(source, fn);

        std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
        JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
        pf_ = pfManager->NewJSPandaFile(pfPtr.release(), filename);

        const File *file = pf_->GetPandaFile();
        const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
        File::EntityId classId = file->GetClassId(typeDesc);
        EXPECT_TRUE(classId.IsValid());

        ClassDataAccessor cda(*file, classId);
        cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
            auto *methodLiteral = new MethodLiteral(mda.GetMethodId());
            methodLiteral->Initialize(pf_.get());
            pf_->SetMethodLiteralToMap(methodLiteral);
            methodLiterals.push_back(methodLiteral);
        });
    }

    void ExecuteAndLoadJSPandaFile(std::string profDir, std::string recordName)
    {
        RuntimeOption option;
        option.SetEnableProfile(true);
        option.SetProfileDir(profDir);
        option.SetLogLevel(LOG_LEVEL::INFO);
        vm_ = JSNApi::CreateJSVM(option);
        JSNApi::EnableUserUncaughtErrorHandler(vm_);

        std::string targetAbcPath = TARGET_ABC_PATH + recordName + ".abc";
        auto result = JSNApi::Execute(vm_, targetAbcPath, recordName, false);
        EXPECT_TRUE(result);

        pf_ = JSPandaFileManager::GetInstance()->FindJSPandaFile(CString(targetAbcPath));

        JSNApi::DestroyJSVM(vm_);
    }

    void ParseRelatedPandaFileMethods(
        PGOProfilerDecoder &loader,
        std::unordered_map<std::string, std::unordered_map<std::string, std::vector<PGOMethodId>>> &methodIdInAp)
    {
        std::shared_ptr<PGOAbcFilePool> abcFilePool = std::make_shared<PGOAbcFilePool>();
        ASSERT_TRUE(loader.LoadFull(abcFilePool));
        for (const auto &recordInfo : loader.GetRecordDetailInfos().GetRecordInfos()) {
            auto recordProfile = recordInfo.first;
            ASSERT_EQ(recordProfile.GetKind(), ProfileType::Kind::RecordClassId);
            if (recordProfile.IsNone()) {
                continue;
            }
            LOG_ECMA(ERROR) << "recordProfile: " << recordProfile.GetTypeString();
            const auto *recordName =
                loader.GetRecordDetailInfos().GetRecordPool()->GetName(recordProfile);
            ASSERT(recordName != nullptr);
            const auto abcNormalizedDesc =
                JSPandaFile::GetNormalizedFileDesc(abcFilePool->GetEntry(recordProfile.GetAbcId())->GetData());
            if (abcNormalizedDesc.empty()) {
                continue;
            }

            const auto *info = recordInfo.second;
            for (const auto &method : info->GetMethodInfos()) {
                // add ap entry info
                methodIdInAp[abcNormalizedDesc.c_str()][recordName].emplace_back(method.first);
            }
        };
    }

    void CheckApMethods(
        std::unordered_map<std::string, std::unordered_map<std::string, std::vector<PGOMethodId>>> &methodIdInAp)
    {
        for (auto abcIter = methodIdInAp.begin(); abcIter != methodIdInAp.end();) {
            std::string fileName(abcIter->first.c_str());
            auto lastDirToken = fileName.find_last_of('/');
            if (lastDirToken != std::string::npos) {
                fileName = fileName.substr(lastDirToken + 1);
            }
            std::unordered_map<std::string, std::vector<PGOMethodId>> &recordMethodList = abcIter->second;
            CheckApMethodsInApFiles(fileName, recordMethodList);
            if (recordMethodList.empty()) {
                abcIter = methodIdInAp.erase(abcIter);
            } else {
                abcIter++;
            }
        }
        ASSERT_TRUE(methodIdInAp.empty());
    }

    void CheckApMethodsInApFiles(const std::string &fileName,
                                 std::unordered_map<std::string, std::vector<PGOMethodId>> &recordMethodList)
    {
        RuntimeOption option;
        vm_ = JSNApi::CreateJSVM(option);
        std::vector<MethodLiteral *> methodLiterals {};
        CreateJSPandaFile(fileName.c_str(), methodLiterals);
        for (auto &methodLiteral : methodLiterals) {
            auto methodName = MethodLiteral::GetRecordName(pf_.get(), methodLiteral->GetMethodId());
            auto recordEntry = recordMethodList.find(methodName.c_str());
            if (recordEntry == recordMethodList.end()) {
                continue;
            }
            for (size_t index = 0; index < recordEntry->second.size(); ++index) {
                if (!(recordEntry->second.at(index) == methodLiteral->GetMethodId())) {
                    continue;
                }
                // Remove matched entry
                recordEntry->second.erase(recordEntry->second.begin() + index);
                if (recordEntry->second.empty()) {
                    recordEntry = recordMethodList.erase(recordEntry);
                }
                break;
            }
        }
        JSNApi::DestroyJSVM(vm_);
    }

    void DestroyJSPandaFile() {
        RuntimeOption option;
        vm_ = JSNApi::CreateJSVM(option);
        JSPandaFileManager::GetInstance()->RemoveJSPandaFile(pf_.get());
        pf_.reset();
        JSNApi::DestroyJSVM(vm_);
    }

    static constexpr uint32_t DECODER_THRESHOLD = 2;
    EcmaVM *vm_ = nullptr;
    bool strictMatch_ = false;
    std::shared_ptr<JSPandaFile> pf_;
};

HWTEST_F_L0(PGOProfilerTest, Sample)
{
    mkdir("ark-profiler/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler/");
    vm_ = JSNApi::CreateJSVM(option);
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile("sample_test.abc", methodLiterals);
    vm_->GetJSThread()->ManagedCodeBegin();
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewSConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "ark-profiler.abc");
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSHandle<Method> method = vm_->GetFactory()->NewSMethod(methodLiterals[0]);
    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    func->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetPGOProfiler()->SetSaveTimestamp(std::chrono::system_clock::now());
    vm_->GetJSThread()->ManagedCodeEnd();
    JSNApi::DestroyJSVM(vm_);
    // Loader
    PGOProfilerDecoder loader("ark-profiler/modules.ap", DECODER_THRESHOLD);
    CString expectRecordName = "sample_test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify({{"ark-profiler.abc", checksum}}));
    ASSERT_FALSE(loader.LoadAndVerify({{"ark-profiler.abc", 123456}}));
    ASSERT_TRUE(!loader.Match(pf_.get(), expectRecordName, methodLiterals[0]->GetMethodId()));
#else
    ASSERT_TRUE(!loader.LoadAndVerify({{"ark-profiler.abc", checksum}}));
    ASSERT_FALSE(loader.LoadAndVerify({{"ark-profiler.abc", 123456}}));
    ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName, methodLiterals[0]->GetMethodId()));
#endif
    unlink("ark-profiler/modules.ap");
    rmdir("ark-profiler/");
}

HWTEST_F_L0(PGOProfilerTest, Sample1)
{
    mkdir("ark-profiler1/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler1/");
    vm_ = JSNApi::CreateJSVM(option);
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile("sample_test.abc", methodLiterals);
    vm_->GetJSThread()->ManagedCodeBegin();
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewSConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "ark-profiler1.abc");
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSHandle<Method> method = vm_->GetFactory()->NewSMethod(methodLiterals[0]);
    JSHandle<Method> method1 = vm_->GetFactory()->NewSMethod(methodLiterals[1]);
    JSHandle<Method> method2 = vm_->GetFactory()->NewSMethod(methodLiterals[2]);
    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    method1->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    method2->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());

    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSFunction> func1 = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method1);
    JSHandle<JSFunction> func2 = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method2);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    func1->SetModule(vm_->GetJSThread(), recordName);
    func2->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetJSThread()->ManagedCodeEnd();
    JSNApi::DestroyJSVM(vm_);

    // Loader
    PGOProfilerDecoder loader("ark-profiler1/modules.ap", DECODER_THRESHOLD);
    CString expectRecordName = "sample_test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify({{"ark-profiler1.abc", checksum}}));
    for (uint32_t idx = 0; idx < 3; idx++) {
        loader.MatchAndMarkMethod(pf_.get(), expectRecordName,
                                  methodLiterals[idx]->GetMethodName(pf_.get(), methodLiterals[idx]->GetMethodId()),
                                  methodLiterals[idx]->GetMethodId());
    }
    ASSERT_TRUE(!loader.Match(pf_.get(), expectRecordName, methodLiterals[0]->GetMethodId()));
    ASSERT_TRUE(!loader.Match(pf_.get(), expectRecordName, methodLiterals[2]->GetMethodId()));
    ASSERT_TRUE(!loader.Match(pf_.get(), expectRecordName, methodLiterals[1]->GetMethodId()));
#else
    ASSERT_TRUE(!loader.LoadAndVerify({{"ark-profiler1.abc", checksum}}));
    ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName, methodLiterals[1]->GetMethodId()));
#endif
    unlink("ark-profiler1/modules.ap");
    rmdir("ark-profiler1/");
}

HWTEST_F_L0(PGOProfilerTest, Sample2)
{
    mkdir("ark-profiler2/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler2/");
    vm_ = JSNApi::CreateJSVM(option);
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile("sample_test.abc", methodLiterals);
    vm_->GetJSThread()->ManagedCodeBegin();
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewSConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "ark-profiler2.abc");

    JSHandle<Method> method = vm_->GetFactory()->NewSMethod(methodLiterals[0]);
    JSHandle<Method> method1 = vm_->GetFactory()->NewSMethod(methodLiterals[1]);

    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    method1->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    JSHandle<JSFunction> func1 = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method1);
    JSHandle<JSTaggedValue> recordName1(vm_->GetFactory()->NewFromStdString("test1"));
    func1->SetModule(vm_->GetJSThread(), recordName1);
    vm_->GetJSThread()->ManagedCodeEnd();
    JSNApi::DestroyJSVM(vm_);

    // Loader
    PGOProfilerDecoder loader("ark-profiler2/modules.ap", DECODER_THRESHOLD);
    CString expectRecordName = "sample_test";
    CString expectRecordName1 = "sample_test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify({{"ark-profiler2.abc", checksum}}));
    for (uint32_t idx = 0; idx < 2; idx++) {
        loader.MatchAndMarkMethod(pf_.get(), expectRecordName,
                                  methodLiterals[idx]->GetMethodName(pf_.get(), methodLiterals[idx]->GetMethodId()),
                                  methodLiterals[idx]->GetMethodId());
        loader.MatchAndMarkMethod(pf_.get(), expectRecordName1,
                                  methodLiterals[idx]->GetMethodName(pf_.get(), methodLiterals[idx]->GetMethodId()),
                                  methodLiterals[idx]->GetMethodId());
    }
    ASSERT_TRUE(!loader.Match(pf_.get(), expectRecordName, methodLiterals[0]->GetMethodId()));
    ASSERT_TRUE(!loader.Match(pf_.get(), expectRecordName1, methodLiterals[1]->GetMethodId()));
#else
    ASSERT_TRUE(!loader.LoadAndVerify({{"ark-profiler2.abc", checksum}}));
    ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName, methodLiterals[0]->GetMethodId()));
    ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName1, methodLiterals[1]->GetMethodId()));
#endif
    unlink("ark-profiler2/modules.ap");
    rmdir("ark-profiler2/");
}

HWTEST_F_L0(PGOProfilerTest, DisEnableSample)
{
    mkdir("ark-profiler3/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(false);
    option.SetProfileDir("ark-profiler3/");
    vm_ = JSNApi::CreateJSVM(option);
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile("sample_test.abc", methodLiterals);
    EXPECT_GE(methodLiterals.size(), 1);
    vm_->GetJSThread()->ManagedCodeBegin();
    JSPandaFileManager::GetInstance()->AddJSPandaFile(pf_);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewSConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    uint32_t checksum = pf_->GetChecksum();
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "sample_test.abc");
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSHandle<Method> method = vm_->GetFactory()->NewSMethod(methodLiterals[0]);

    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("sample_test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetJSThread()->ManagedCodeEnd();
    JSNApi::DestroyJSVM(vm_);

    // Loader
    ASSERT_FALSE(FileExist("ark-profiler3/modules.ap"));
    PGOProfilerDecoder loader("ark-profiler3/modules.ap", DECODER_THRESHOLD);
    ASSERT_TRUE(!loader.LoadAndVerify({{"sample_test.abc", checksum}}));
    CString expectRecordName = "sample_test";
    ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName, methodLiterals[0]->GetMethodId()));
    rmdir("ark-profiler3/");
}

HWTEST_F_L0(PGOProfilerTest, PGOProfilerManagerInvalidPath)
{
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler4");
    vm_ = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
    JSNApi::DestroyJSVM(vm_);
}

HWTEST_F_L0(PGOProfilerTest, PGOProfilerManagerInitialize)
{
    RuntimeOption option;
    option.SetEnableProfile(true);
    // outDir is empty
    option.SetProfileDir("");
    vm_ = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSNApi::DestroyJSVM(vm_);
}

HWTEST_F_L0(PGOProfilerTest, PGOProfilerManagerSample)
{
    RuntimeOption option;
    option.SetEnableProfile(true);
    char currentPath[PATH_MAX + 2];
    if (memset_s(currentPath, PATH_MAX, 1, PATH_MAX) != EOK) {
        ASSERT_TRUE(false);
    }
    currentPath[PATH_MAX + 1] = '\0';
    option.SetProfileDir(currentPath);
    vm_ = JSNApi::CreateJSVM(option);
    vm_->GetJSThread()->ManagedCodeBegin();
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "");
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    // RecordName is hole
    MethodLiteral *methodLiteral = new MethodLiteral(EntityId(61));
    JSHandle<Method> method = vm_->GetFactory()->NewSMethod(methodLiteral);
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    func->SetModule(vm_->GetJSThread(), JSTaggedValue::Hole());
    vm_->GetJSThread()->ManagedCodeEnd();
    JSNApi::DestroyJSVM(vm_);

    PGOProfilerDecoder loader("", DECODER_THRESHOLD);
    // path is empty()
    ASSERT_TRUE(loader.LoadAndVerify({{"", checksum}}));
    // path size greater than PATH_MAX
    char path[PATH_MAX + 1] = {'0'};
    PGOProfilerDecoder loader1(path, 4);
    ASSERT_TRUE(!loader1.LoadAndVerify({{"", checksum}}));
}

HWTEST_F_L0(PGOProfilerTest, PGOProfilerDoubleVM)
{
    mkdir("ark-profiler5/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    // outDir is empty
    option.SetProfileDir("ark-profiler5/");
    vm_ = JSNApi::CreateJSVM(option);
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile("sample_test.abc", methodLiterals);
    EXPECT_GE(methodLiterals.size(), 2);  // number of methods
    vm_->GetJSThread()->ManagedCodeBegin();
    JSPandaFileManager::GetInstance()->AddJSPandaFile(pf_);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewSConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    uint32_t checksum = pf_->GetChecksum();
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "sample_test.abc");
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
    // worker vm read profile enable from PGOProfilerManager singleton
    option.SetEnableProfile(false);

    std::thread t1([&]() {
        auto vm2 = JSNApi::CreateJSVM(option);
        vm2->GetJSThread()->ManagedCodeBegin();
        JSHandle<ConstantPool> constPool2 = vm2->GetFactory()->NewSConstantPool(4);
        constPool2->SetJSPandaFile(pf_.get());
        PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "sample_test.abc");
        ASSERT_TRUE(vm2 != nullptr) << "Cannot create Runtime";

        JSHandle<Method> method = vm2->GetFactory()->NewSMethod(methodLiterals[0]);
        method->SetConstantPool(vm2->GetJSThread(), constPool2.GetTaggedValue());
        JSHandle<JSFunction> func = vm2->GetFactory()->NewJSFunction(vm2->GetGlobalEnv(), method);
        JSHandle<JSTaggedValue> recordName(vm2->GetFactory()->NewFromStdString("sample_test"));
        func->SetModule(vm2->GetJSThread(), recordName);
        vm2->GetJSThread()->ManagedCodeEnd();
        JSNApi::DestroyJSVM(vm2);
    });
    {
        ThreadSuspensionScope scope(vm_->GetJSThread());
        t1.join();
    }
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("sample_test"));
    JSHandle<Method> method1 = vm_->GetFactory()->NewSMethod(methodLiterals[0]);
    JSHandle<Method> method2 = vm_->GetFactory()->NewSMethod(methodLiterals[1]);
    method1->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    method2->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSFunction> func1 = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method1);
    JSHandle<JSFunction> func2 = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method2);
    JSHandle<JSTaggedValue> recordName1(vm_->GetFactory()->NewFromStdString("sample_test"));
    func1->SetModule(vm_->GetJSThread(), recordName);
    func2->SetModule(vm_->GetJSThread(), recordName);

    vm_->GetJSThread()->ManagedCodeEnd();
    JSNApi::DestroyJSVM(vm_);

    PGOProfilerDecoder loader("ark-profiler5/profiler", DECODER_THRESHOLD);
    mkdir("ark-profiler5/profiler", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    ASSERT_TRUE(!loader.LoadAndVerify({{"sample_test.abc", checksum}}));
    CString expectRecordName = "sample_test";
    ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName, methodLiterals[1]->GetMethodId()));

    PGOProfilerDecoder loader1("ark-profiler5/modules.ap", DECODER_THRESHOLD);
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader1.LoadAndVerify({{"sample_test.abc", checksum}}));
    ASSERT_TRUE(!loader1.Match(pf_.get(), expectRecordName, methodLiterals[1]->GetMethodId()));
#else
    ASSERT_TRUE(!loader1.LoadAndVerify({{"sample_test.abc", checksum}}));
    ASSERT_TRUE(loader1.Match(pf_.get(), expectRecordName, methodLiterals[1]->GetMethodId()));
#endif

    unlink("ark-profiler5/modules.ap");
    rmdir("ark-profiler5/profiler");
    rmdir("ark-profiler5/");
}

HWTEST_F_L0(PGOProfilerTest, PGOProfilerDecoderNoHotMethod)
{
    mkdir("ark-profiler8/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler8/");
    vm_ = JSNApi::CreateJSVM(option);
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile("sample_test.abc", methodLiterals);
    EXPECT_GE(methodLiterals.size(), 1);  // number of methods
    vm_->GetJSThread()->ManagedCodeBegin();
    JSPandaFileManager::GetInstance()->AddJSPandaFile(pf_);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewSConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    uint32_t checksum = pf_->GetChecksum();
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "sample_test.abc");

    JSHandle<Method> method = vm_->GetFactory()->NewSMethod(methodLiterals[0]);

    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("sample_test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetJSThread()->ManagedCodeEnd();
    JSNApi::DestroyJSVM(vm_);

    PGOProfilerDecoder loader("ark-profiler8/modules.ap", DECODER_THRESHOLD);
    CString expectRecordName = "sample_test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify({{"sample_test.abc", checksum}}));
    ASSERT_TRUE(!loader.Match(pf_.get(), expectRecordName, methodLiterals[0]->GetMethodId()));
#else
    ASSERT_TRUE(!loader.LoadAndVerify({{"sample_test.abc", checksum}}));
    ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName, methodLiterals[0]->GetMethodId()));
#endif

    unlink("ark-profiler8/modules.ap");
    rmdir("ark-profiler8/");
}

HWTEST_F_L0(PGOProfilerTest, PGOProfilerPostTask)
{
    std::stringstream sourceStream;
    sourceStream << "  .language ECMAScript" << std::endl;
    for (uint32_t funcIdx = 0; funcIdx < 100; funcIdx++) {
        sourceStream << "  .function void foo" << std::to_string(funcIdx) << "(any a0, any a1, any a2) {}" << std::endl;
    }
    mkdir("ark-profiler9/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler9/");
    vm_ = JSNApi::CreateJSVM(option);
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile(sourceStream.str().c_str(), "ark-profiler9.abc", methodLiterals);
    EXPECT_EQ(methodLiterals.size(), 100);  // number of methods
    vm_->GetJSThread()->ManagedCodeBegin();
    JSPandaFileManager::GetInstance()->AddJSPandaFile(pf_);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewSConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    uint32_t checksum = pf_->GetChecksum();
    PGOProfilerManager::GetInstance()->SetApGenMode(ApGenMode::OVERWRITE);
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "ark-profiler9.abc");

    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("ark-profiler9.abc"));
    for (int i = 61; i < 91; i++) {
        JSHandle<Method> method = vm_->GetFactory()->NewSMethod(methodLiterals[i]);
        method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
        JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
        func->SetModule(vm_->GetJSThread(), recordName);
    }

    vm_->GetJSThread()->ManagedCodeEnd();
    JSNApi::DestroyJSVM(vm_);

    PGOProfilerDecoder loader("ark-profiler9/modules.ap", DECODER_THRESHOLD);
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify({{"ark-profiler9.abc", checksum}}));
#else
    ASSERT_TRUE(!loader.LoadAndVerify({{"ark-profiler9.abc", checksum}}));
#endif
    CString expectRecordName = "ark-profiler9.abc";
    for (int i = 0; i < 100; i++) {
        EntityId methodId = methodLiterals[i]->GetMethodId();
        loader.MatchAndMarkMethod(pf_.get(), expectRecordName, methodLiterals[i]->GetMethodName(pf_.get(), methodId),
                                  methodId);
    }
    for (int i = 61; i < 91; i++) {
#if defined(SUPPORT_ENABLE_ASM_INTERP)
        if (i % 3 == 0) {
            ASSERT_TRUE(!loader.Match(pf_.get(), expectRecordName, methodLiterals[i]->GetMethodId()));
        } else {
            ASSERT_TRUE(!loader.Match(pf_.get(), expectRecordName, methodLiterals[i]->GetMethodId()));
#else
        if (i % 3 == 0) {
            ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName, methodLiterals[i]->GetMethodId()));
        } else {
            ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName, methodLiterals[i]->GetMethodId()));
#endif
        }
    }

    unlink("ark-profiler9/modules.ap");
    rmdir("ark-profiler9/");
}

HWTEST_F_L0(PGOProfilerTest, FailResetProfilerInWorker)
{
    mkdir("ark-profiler12/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    // Although enableProfile is set in option, but it will not work when isWorker is set.
    option.SetEnableProfile(true);
    option.SetIsWorker();
    option.SetProfileDir("ark-profiler12/");
    // PgoProfiler is disabled as default.
    vm_ = JSNApi::CreateJSVM(option);
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile("sample_test.abc", methodLiterals);
    EXPECT_GE(methodLiterals.size(), 1);  // number of methods
    vm_->GetJSThread()->ManagedCodeBegin();
    JSPandaFileManager::GetInstance()->AddJSPandaFile(pf_);
    uint32_t checksum = pf_->GetChecksum();
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "sample_test.abc");
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSHandle<Method> method = vm_->GetFactory()->NewSMethod(methodLiterals[0]);

    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewSConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("sample_test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetJSThread()->ManagedCodeEnd();
    JSNApi::DestroyJSVM(vm_);

    // Loader
    ASSERT_FALSE(FileExist("ark-profiler12/modules.ap"));
    PGOProfilerDecoder loader("ark-profiler12/modules.ap", DECODER_THRESHOLD);
    ASSERT_TRUE(!loader.LoadAndVerify({{"sample_test.abc", checksum}}));
    CString expectRecordName = "sample_test";
    ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName, methodLiterals[0]->GetMethodId()));
    rmdir("ark-profiler12/");
}

#if defined(SUPPORT_ENABLE_ASM_INTERP)
HWTEST_F_L0(PGOProfilerTest, ProfileCallTest)
{
    mkdir("ark-profiler13/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "call_test";
    ExecuteAndLoadJSPandaFile("ark-profiler13/", targetRecordName);
    ASSERT_NE(pf_, nullptr);
    uint32_t checksum = pf_->GetChecksum();
    ;

    // Loader
    PGOProfilerDecoder decoder("ark-profiler13/modules.ap", 1);
    PGOProfilerDecoder decoder1("ark-profiler13/modules.ap", 10);
    PGOProfilerDecoder decoder2("ark-profiler13/modules.ap", 11000);
    ASSERT_TRUE(decoder.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
    ASSERT_TRUE(decoder1.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
    ASSERT_TRUE(decoder2.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        decoder1.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        decoder2.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        // ASSERT_TRUE(decoder.Match(pf_.get(), targetRecordName, methodId))
        if (std::string(methodName) == "Test") {
            ASSERT_TRUE(decoder1.Match(pf_.get(), targetRecordName, methodId));
            ASSERT_TRUE(decoder2.Match(pf_.get(), targetRecordName, methodId));
        } else {
            // ASSERT_TRUE(decoder1.Match(pf_.get(), targetRecordName, methodId))
            // ASSERT_TRUE(decoder2.Match(pf_.get(), targetRecordName, methodId))
        }
    }
    unlink("ark-profiler13/modules.ap");
    rmdir("ark-profiler13/");
}

HWTEST_F_L0(PGOProfilerTest, UseClassTypeTest)
{
    mkdir("ark-profiler14/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "class_test";
    ExecuteAndLoadJSPandaFile("ark-profiler14/", targetRecordName);
    ASSERT_NE(pf_, nullptr);
    uint32_t checksum = pf_->GetChecksum();

    // Loader
    PGOProfilerDecoder decoder("ark-profiler14/modules.ap", 1);
    ASSERT_TRUE(decoder.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        ASSERT_TRUE(!decoder.Match(pf_.get(), targetRecordName, methodId));
        auto callback = [methodName](uint32_t offset, const PGOType *type) {
            ASSERT_NE(offset, 0);
            if (type->IsScalarOpType()) {
            } else if (type->IsRwOpType()) {
                auto pgoRWOpType = *reinterpret_cast<const PGORWOpType *>(type);
                if (std::string(methodName) == "Foot" || std::string(methodName) == "Arm") {
                    ASSERT_TRUE(pgoRWOpType.GetCount() == 1);
                } else if (std::string(methodName) == "foo" || std::string(methodName) == "Body") {
                    ASSERT_TRUE(pgoRWOpType.GetCount() == 3);
                }
            } else {
                ASSERT_TRUE(true);
            }
        };
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral, callback);
    }
    unlink("ark-profiler14/modules.ap");
    rmdir("ark-profiler14/");
}

HWTEST_F_L0(PGOProfilerTest, DefineClassTypeTest)
{
    mkdir("ark-profiler15/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "class_test";
    ExecuteAndLoadJSPandaFile("ark-profiler15/", targetRecordName);
    ASSERT_NE(pf_, nullptr);
    uint32_t checksum = pf_->GetChecksum();

    // Loader
    PGOProfilerDecoder decoder("ark-profiler15/modules.ap", 1);
    ASSERT_TRUE(decoder.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        ASSERT_TRUE(!decoder.Match(pf_.get(), targetRecordName, methodId));
        auto callback = [methodName, &decoder, jsPandaFile = pf_](uint32_t offset, const PGOType *type) {
            ASSERT_NE(offset, 0);
            if (type->IsScalarOpType()) {
                auto sampleType = *reinterpret_cast<const PGOSampleType *>(type);
                if (sampleType.IsProfileType()) {
                    ASSERT_EQ(std::string(methodName), "func_main_0");
                    PGOHClassTreeDesc *desc;
                    if (!decoder.GetHClassTreeDesc(sampleType, &desc)) {
                        return;
                    }
                    return;
                    auto classId = EntityId(sampleType.GetProfileType().GetId());
                    auto className = MethodLiteral::GetMethodName(jsPandaFile.get(), classId);
                    if (std::string(className) == "Arm") {
                        auto superClassId = EntityId(desc->GetProfileType().GetId());
                        auto superClassName = MethodLiteral::GetMethodName(jsPandaFile.get(), superClassId);
                        ASSERT_EQ(std::string(superClassName), "Body");
                    } else if (std::string(className) == "Foot") {
                        auto superClassId = EntityId(desc->GetProfileType().GetId());
                        auto superClassName = MethodLiteral::GetMethodName(jsPandaFile.get(), superClassId);
                        ASSERT_EQ(std::string(superClassName), "Body");
                    } else {
                        ASSERT_EQ(desc->GetProfileType().GetRaw(), 0);
                    }
                }
            }
        };
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral, callback);
    }
    unlink("ark-profiler15/modules.ap");
    rmdir("ark-profiler15/");
}

HWTEST_F_L0(PGOProfilerTest, OpTypeTest)
{
    mkdir("ark-profiler16/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "op_type_test";
    ExecuteAndLoadJSPandaFile("ark-profiler16/", targetRecordName);
    ASSERT_NE(pf_, nullptr);
    uint32_t checksum = pf_->GetChecksum();

    // Loader
    PGOProfilerDecoder decoder("ark-profiler16/modules.ap", 1);
    ASSERT_TRUE(decoder.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
    std::string types[17] = {"1", "5", "4", "4", "4", "4", "4", "4", "5", "4", "4", "1", "1", "4", "5", "1", "1"};
    int index = 0;
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        if (std::string(methodName) != "sun" && std::string(methodName) != "sun1") {
            decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
            ASSERT_TRUE(!decoder.Match(pf_.get(), targetRecordName, methodId));
        }
        auto callback = [methodName, types, &index](uint32_t offset, const PGOType *type) {
            ASSERT_NE(offset, 0);
            if (type->IsScalarOpType()) {
                auto sampleType = *reinterpret_cast<const PGOSampleType *>(type);
                if (sampleType.IsProfileType()) {
                    return;
                }
                if (std::string(methodName) == "advance") {
                    if (sampleType.GetWeight() > 0) {
                        auto trueWeight = sampleType.GetWeight() >> 10;
                        auto falseWeight = sampleType.GetWeight() & 0x7FF;
                        auto primitiveType = sampleType.GetPrimitiveType();
                        ASSERT_GT(trueWeight, falseWeight);
                        ASSERT_EQ(static_cast<uint32_t>(primitiveType), PGOSampleType::IntType());
                    } else {
                        ASSERT_EQ(sampleType.GetTypeString(), types[index++]);
                    }
                }
            }
        };
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral, callback);
    }
    unlink("ark-profiler16/modules.ap");
    rmdir("ark-profiler16/");
}

HWTEST_F_L0(PGOProfilerTest, ArrayProfileTest)
{
    mkdir("ark-profiler18/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "array_test";
    ExecuteAndLoadJSPandaFile("ark-profiler18/", targetRecordName);
    ASSERT_NE(pf_, nullptr);
    uint32_t checksum = pf_->GetChecksum();

    // Loader
    PGOProfilerDecoder decoder("ark-profiler18/modules.ap", 1);
    ASSERT_TRUE(decoder.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        ASSERT_TRUE(!decoder.Match(pf_.get(), targetRecordName, methodId));
        auto callback = [methodName, &decoder, jsPandaFile = pf_](uint32_t offset, const PGOType *type) {
            if (type->IsScalarOpType()) {
                auto sampleType = *reinterpret_cast<const PGOSampleType *>(type);
                if (sampleType.IsProfileType()) {
                    ASSERT_EQ(std::string(methodName), "func_main_0");
                    PGOHClassTreeDesc *desc;
                    if (!decoder.GetHClassTreeDesc(sampleType, &desc)) {
                        return;
                    }
                }
            } else if (type->IsRwOpType()) {
                auto pgoRWOpType = *reinterpret_cast<const PGORWOpType *>(type);
                if (std::string(methodName) == "foo") {
                    ASSERT_TRUE(pgoRWOpType.GetCount() == 3);
                    auto classType = pgoRWOpType.GetObjectInfo(0).GetProfileType();
                    ASSERT_TRUE(classType.IsBuiltinsArray());
                    ASSERT_EQ(classType.GetElementsKindBeforeTransition(), ElementsKind::NUMBER);

                    classType = pgoRWOpType.GetObjectInfo(1).GetProfileType();
                    ASSERT_TRUE(classType.IsBuiltinsArray());
                    ASSERT_EQ(classType.GetElementsKindBeforeTransition(), ElementsKind::HOLE_INT);

                    classType = pgoRWOpType.GetObjectInfo(2).GetProfileType();
                    ASSERT_TRUE(classType.IsBuiltinsArray());
                    ASSERT_EQ(classType.GetElementsKindBeforeTransition(), ElementsKind::TAGGED);
                } else if (std::string(methodName) == "foo1") {
                    ASSERT_TRUE(pgoRWOpType.GetCount() == 2);
                    auto classType = pgoRWOpType.GetObjectInfo(0).GetProfileType();
                    ASSERT_TRUE(classType.IsBuiltinsArray());
                    ASSERT_EQ(classType.GetElementsKindBeforeTransition(), ElementsKind::NUMBER);

                    classType = pgoRWOpType.GetObjectInfo(1).GetProfileType();
                    ASSERT_TRUE(classType.IsBuiltinsArray());
                    ASSERT_EQ(classType.GetElementsKindBeforeTransition(), ElementsKind::TAGGED);
                } else if (std::string(methodName) == "foo2") {
                    ASSERT_TRUE(pgoRWOpType.GetCount() == 1);
                    auto classType = pgoRWOpType.GetObjectInfo(0).GetProfileType();
                    ASSERT_TRUE(classType.IsBuiltinsArray());
                    ASSERT_EQ(classType.GetElementsKindBeforeTransition(), ElementsKind::HOLE_TAGGED);
                }
            }
        };
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral, callback);
    }
    unlink("ark-profiler18/modules.ap");
    rmdir("ark-profiler18/");
}

HWTEST_F_L0(PGOProfilerTest, ObjectLiteralProfileTest)
{
    mkdir("ark-profiler20/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "object_literal";
    ExecuteAndLoadJSPandaFile("ark-profiler20/", targetRecordName);
    ASSERT_NE(pf_, nullptr);
    uint32_t checksum = pf_->GetChecksum();

    // Loader
    PGOProfilerDecoder decoder("ark-profiler20/modules.ap", 1);
    ASSERT_TRUE(decoder.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        ASSERT_TRUE(!decoder.Match(pf_.get(), targetRecordName, methodId));
        auto callback = [methodName, &decoder, jsPandaFile = pf_](uint32_t offset, const PGOType *type) {
            if (type->IsScalarOpType()) {
                auto sampleType = *reinterpret_cast<const PGOSampleType *>(type);
                if (sampleType.IsProfileType()) {
                    ASSERT_EQ(std::string(methodName), "func_main_0");
                    PGOHClassTreeDesc *desc;
                    if (!decoder.GetHClassTreeDesc(sampleType, &desc)) {
                        return;
                    }
                }
            } else if (type->IsRwOpType()) {
                auto pgoRWOpType = *reinterpret_cast<const PGORWOpType *>(type);
                if (std::string(methodName) == "foo") {
                    ASSERT_TRUE(pgoRWOpType.GetCount() == 2);
                    auto classType = PGOSampleType(pgoRWOpType.GetObjectInfo(0).GetProfileType());
                    PGOHClassTreeDesc *desc;
                    ASSERT_TRUE(decoder.GetHClassTreeDesc(classType, &desc));

                    classType = PGOSampleType(pgoRWOpType.GetObjectInfo(1).GetProfileType());
                    ASSERT_TRUE(decoder.GetHClassTreeDesc(classType, &desc));
                }
            }
        };
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral, callback);
    }
    unlink("ark-profiler20/modules.ap");
    rmdir("ark-profiler20/");
}

HWTEST_F_L0(PGOProfilerTest, ArraySizeProfileTest)
{
    mkdir("ark-profiler21/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "array_size_test";
    ExecuteAndLoadJSPandaFile("ark-profiler21/", targetRecordName);
    ASSERT_NE(pf_, nullptr);
    uint32_t checksum = pf_->GetChecksum();

    // Loader
    PGOProfilerDecoder decoder("ark-profiler21/modules.ap", 1);
    ASSERT_TRUE(decoder.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        ASSERT_TRUE(!decoder.Match(pf_.get(), targetRecordName, methodId));
        auto callback = [methodName, jsPandaFile = pf_](uint32_t offset, const PGOType *type) {
            if (type->IsDefineOpType()) {
                auto defineOptype = reinterpret_cast<const PGODefineOpType *>(type);
                if (std::string(methodName) == "foo") {
                    ASSERT_EQ(defineOptype->GetElementsLength(), 4);
                } else if (std::string(methodName) == "foo1") {
                    ASSERT_EQ(defineOptype->GetElementsLength(), 12);
                } else if (std::string(methodName) == "foo2") {
                    ASSERT_EQ(defineOptype->GetElementsLength(), 12);
                }
            }
        };
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral, callback);
    }
    unlink("ark-profiler21/modules.ap");
    rmdir("ark-profiler21/");
}

HWTEST_F_L0(PGOProfilerTest, StringEqualProfileTest)
{
    mkdir("ark-profiler22/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "string_equal";
    ExecuteAndLoadJSPandaFile("ark-profiler22/", targetRecordName);
    ASSERT_NE(pf_, nullptr);
    uint32_t checksum = pf_->GetChecksum();

    // Loader
    PGOProfilerDecoder decoder("ark-profiler22/modules.ap", 1);
    ASSERT_TRUE(decoder.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        ASSERT_TRUE(!decoder.Match(pf_.get(), targetRecordName, methodId));
        auto callback = [methodName, jsPandaFile = pf_](uint32_t offset, const PGOType *type) {
            if (type->IsScalarOpType()) {
                auto sampleType = *reinterpret_cast<const PGOSampleType *>(type);
                if (sampleType.IsProfileType()) {
                    return;
                }
                if (std::string(methodName) == "foo1" || std::string(methodName) == "foo2") {
                    auto primitiveType = sampleType.GetPrimitiveType();
                    ASSERT_EQ(static_cast<uint32_t>(primitiveType), PGOSampleType::StringType());
                }
            }
        };
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral, callback);
    }
    unlink("ark-profiler22/modules.ap");
    rmdir("ark-profiler22/");
}

HWTEST_F_L0(PGOProfilerTest, BuiltinsTest)
{
    mkdir("ark-profiler23/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "builtins_test";
    ExecuteAndLoadJSPandaFile("ark-profiler23/", targetRecordName);
    ASSERT_NE(pf_, nullptr);
    uint32_t checksum = pf_->GetChecksum();

    // Loader
    PGOProfilerDecoder decoder("ark-profiler23/modules.ap", 1);
    ASSERT_TRUE(decoder.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        if (std::string(methodName) != "ArrayList") {
            decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
            ASSERT_TRUE(!decoder.Match(pf_.get(), targetRecordName, methodId));
        }
        auto callback = [methodName](uint32_t offset, const PGOType *type) {
            ASSERT_NE(offset, 0);
            if (type->IsRwOpType() && std::string(methodName) == "A") {
                auto pgoRWOpType = *reinterpret_cast<const PGORWOpType *>(type);
                ASSERT_TRUE(pgoRWOpType.GetCount() == 1);
            }
        };
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral, callback);
    }
    unlink("ark-profiler23/modules.ap");
    rmdir("ark-profiler23/");
}

#endif

#if defined(SUPPORT_ENABLE_ASM_INTERP)
HWTEST_F_L0(PGOProfilerTest, FileConsistencyCheck)
{
    mkdir("ark-profiler17/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "sample_test";
    ExecuteAndLoadJSPandaFile("ark-profiler17/", targetRecordName);
    ASSERT_NE(pf_, nullptr);
    uint32_t checksum = pf_->GetChecksum();

    // write to corrupt the ap file's consistency
    std::ofstream fWriter("ark-profiler17/modules.ap", std::fstream::app);

    fWriter.write(reinterpret_cast<char *>(&checksum), sizeof(checksum));
    fWriter.seekp(100);
    fWriter.write(reinterpret_cast<char *>(&checksum), sizeof(checksum));
    fWriter.close();

    // Loader
    PGOProfilerDecoder loader("ark-profiler17/modules.ap", DECODER_THRESHOLD);
    ASSERT_FALSE(loader.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
    unlink("ark-profiler17/modules.ap");
    rmdir("ark-profiler17/");
}

HWTEST_F_L0(PGOProfilerTest, MergeApSelfTwice)
{
    mkdir("ark-profiler18/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "op_type_test";
    ExecuteAndLoadJSPandaFile("ark-profiler18/", targetRecordName);
    ASSERT_NE(pf_, nullptr);

    // Loader
    PGOProfilerDecoder decoder("ark-profiler18/modules_merge.ap", 1);
    PGOProfilerDecoder decoderSingle("ark-profiler18/modules.ap", 1);
    ASSERT_TRUE(PGOProfilerManager::MergeApFiles("ark-profiler18/modules.ap:ark-profiler18/modules.ap",
                                                 "ark-profiler18/modules_merge.ap", 1, ApGenMode::OVERWRITE));
    ASSERT_TRUE(decoder.LoadFull());
    ASSERT_TRUE(decoderSingle.LoadFull());

    auto doubleCount =
        decoder.GetRecordDetailInfos().GetRecordInfos().begin()->second->GetMethodInfos().begin()->second->GetCount();
    auto singleCount = decoderSingle.GetRecordDetailInfos()
                           .GetRecordInfos()
                           .begin()
                           ->second->GetMethodInfos()
                           .begin()
                           ->second->GetCount();
    ASSERT_EQ(doubleCount, singleCount + singleCount);

    unlink("ark-profiler18/modules.ap");
    unlink("ark-profiler18/modules_merge.ap");
    rmdir("ark-profiler18/");
}
#endif

HWTEST_F_L0(PGOProfilerTest, RuntimeMerge)
{
    mkdir("ark-profiler19/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    ExecuteAndLoadJSPandaFile("ark-profiler19/", "truck");
    ExecuteAndLoadJSPandaFile("ark-profiler19/", "call_test");
    ExecuteAndLoadJSPandaFile("ark-profiler19/", "truck");

    // Loader
    PGOProfilerDecoder loader("ark-profiler19/modules.ap", DECODER_THRESHOLD);
    CString expectRecordName = "sample_test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<PGOMethodId>>> methodIdInAp;
    ParseRelatedPandaFileMethods(loader, methodIdInAp);
    ASSERT_EQ(methodIdInAp.size(), 3);
    CheckApMethods(methodIdInAp);
#else
    uint32_t checksum = pf_->GetChecksum();
    ASSERT_TRUE(!loader.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
#endif
    unlink("ark-profiler19/modules.ap");
    rmdir("ark-profiler19/");
}

HWTEST_F_L0(PGOProfilerTest, ProfdumpMerge)
{
    mkdir("ark-profiler20/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    ExecuteAndLoadJSPandaFile("ark-profiler20/merge_file_1.ap", "merge_file_1");
    ExecuteAndLoadJSPandaFile("ark-profiler20/merge_file_2.ap", "merge_file_2");
    ExecuteAndLoadJSPandaFile("ark-profiler20/merge_file_3.ap", "merge_file_3");

    // Loader
    PGOProfilerDecoder loader("ark-profiler20/merged.ap", DECODER_THRESHOLD);
    ASSERT_TRUE(PGOProfilerManager::MergeApFiles(
        "ark-profiler20/merge_file_1.ap:ark-profiler20/merge_file_2.ap:ark-profiler20/merge_file_3.ap",
        "ark-profiler20/merged.ap", 1, ApGenMode::OVERWRITE));

    CString expectRecordName = "sample_test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<PGOMethodId>>> methodIdInAp;
    ParseRelatedPandaFileMethods(loader, methodIdInAp);
    ASSERT_EQ(methodIdInAp.size(), 3);
    CheckApMethods(methodIdInAp);
#else
    uint32_t checksum = pf_->GetChecksum();
    ASSERT_TRUE(!loader.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
#endif
    unlink("ark-profiler20/merge_file_1.ap");
    unlink("ark-profiler20/merge_file_2.ap");
    unlink("ark-profiler20/merge_file_3.ap");
    unlink("ark-profiler20/merged.ap");
    rmdir("ark-profiler20/");
}

HWTEST_F_L0(PGOProfilerTest, ApVersionMatchCheck)
{
    mkdir("ark-ApVersionMatchCheck/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "sample_test";
    ExecuteAndLoadJSPandaFile("ark-ApVersionMatchCheck/", targetRecordName);
    ASSERT_NE(pf_, nullptr);

    auto info = std::make_shared<PGOInfo>(DECODER_THRESHOLD);
    info->GetHeaderPtr()->SetVersion(PGOProfilerHeader::PROFILE_TYPE_WITH_ABC_ID_MINI_VERSION);
    PGOProfilerEncoder encoder("ark-ApVersionMatchCheck/modules.ap", PGOProfilerEncoder::ApGenMode::MERGE);
    encoder.Save(info);

    PGOProfilerDecoder decoder("ark-ApVersionMatchCheck/modules.ap", DECODER_THRESHOLD);
    PGOProfilerHeader::SetStrictMatch(true);
    ASSERT_FALSE(decoder.LoadFull());
    PGOProfilerHeader::SetStrictMatch(false);
    ASSERT_TRUE(decoder.LoadFull());

    unlink("ark-ApVersionMatchCheck/modules.ap");
    unlink("ark-ApVersionMatchCheck/");
}

HWTEST_F_L0(PGOProfilerTest, TypedArrayOnHeap)
{
    mkdir("ark-profiler24/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "typedarray_length";
    ExecuteAndLoadJSPandaFile("ark-profiler24/", targetRecordName);
    ASSERT_NE(pf_, nullptr);
    uint32_t checksum = pf_->GetChecksum();

    // Loader
    PGOProfilerDecoder decoder("ark-profiler24/modules.ap", 1);
    ASSERT_TRUE(decoder.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        auto callback = [methodName](uint32_t offset, const PGOType *type) {
            ASSERT_NE(offset, 0);
            if (type->IsRwOpType() && std::string(methodName) == "test") {
                auto pgoRWOpType = *reinterpret_cast<const PGORWOpType *>(type);
                ASSERT_TRUE(pgoRWOpType.GetCount() == 1);
            }
        };
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral, callback);
    }
    unlink("ark-profiler24/modules.ap");
    rmdir("ark-profiler24/");
}

HWTEST_F_L0(PGOProfilerTest, ProfileTypeConstructor)
{
    mkdir("ark-profiler25/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "typedarray_length";
    ExecuteAndLoadJSPandaFile("ark-profiler25/", targetRecordName);
    ASSERT_NE(pf_, nullptr);
    ApEntityId inValidId = 555;
    ApEntityId validId = 64;
    // Loader
    PGOProfilerDecoder decoder("ark-profiler25/modules.ap", 1);
    ASSERT_TRUE(decoder.LoadFull());

    auto invalidRes = ProfileTypeRef(inValidId);
    auto expectFalse = ProfileType::CreateFromProfileTypeRef(decoder.GetRecordDetailInfos(), invalidRes);
    EXPECT_FALSE(expectFalse.has_value());

    auto validRes = ProfileTypeRef(validId);
    auto expectTrue = ProfileType::CreateFromProfileTypeRef(decoder.GetRecordDetailInfos(), validRes);
    EXPECT_TRUE(expectTrue.has_value());
    unlink("ark-profiler25/modules.ap");
    rmdir("ark-profiler25/");
}

HWTEST_F_L0(PGOProfilerTest, CompatibleWithAOTFileTest)
{
    constexpr uint32_t CHECKSUM = 1;
    PGOProfilerDecoder decoder("", DECODER_THRESHOLD);
    EXPECT_TRUE(decoder.LoadAndVerify({{"", CHECKSUM}}));
    EXPECT_FALSE(decoder.IsCompatibleWithAOTFile());
}

HWTEST_F_L0(PGOProfilerTest, ExternalMethodLiteralTest)
{
    mkdir("ark-profiler26/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "typedarray_length";
    ExecuteAndLoadJSPandaFile("ark-profiler26/", targetRecordName);
    ASSERT_NE(pf_, nullptr);
    uint32_t checksum = pf_->GetChecksum();

    PGOProfilerDecoder decoder("ark-profiler26/modules.ap", DECODER_THRESHOLD);
    ASSERT_TRUE(decoder.LoadAndVerify({{pf_->GetNormalizedFileDesc(), checksum}}));
    auto callback = []([[maybe_unused]] uint32_t offset, [[maybe_unused]] const PGOType *type) { EXPECT_TRUE(false); };
    decoder.GetTypeInfo(pf_.get(), targetRecordName, nullptr, callback);
    unlink("ark-profiler26/modules.ap");
    rmdir("ark-profiler26/");
}

HWTEST_F_L0(PGOProfilerTest, PGOObjectInfoOperatorLessThanTest)
{
    constexpr uint64_t rawTypeLess = 0;
    constexpr uint64_t rawTypeGreater = 1;

    ProfileType profileTypeLess(rawTypeLess);
    ProfileType profileTypeGreater(rawTypeGreater);

    PGOObjectInfo objectInfoLess(profileTypeLess, profileTypeGreater, profileTypeGreater,
                                 profileTypeGreater, profileTypeGreater, profileTypeGreater, PGOSampleType());
    PGOObjectInfo objectInfoGreater(profileTypeGreater, profileTypeLess, profileTypeGreater,
                                    profileTypeGreater, profileTypeGreater, profileTypeGreater, PGOSampleType());

    EXPECT_TRUE(objectInfoLess < objectInfoGreater);
    EXPECT_FALSE(objectInfoGreater < objectInfoLess);
}

HWTEST_F_L0(PGOProfilerTest, PGODisableWithAOTFileWorkloadTest)
{
    mkdir("ark-profiler27/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream fWriter("ark-profiler27/tmp.an", std::fstream::app);
    fWriter.close();
    const char *targetRecordName = "typedarray_length";
    std::string targetAbcPath = std::string(TARGET_ABC_PATH) + targetRecordName + ".abc";
    // Force set in advance to simulate AOT/JIT enable list check is passed in UT
    ecmascript::AnFileDataManager::GetInstance()->SetEnable(true);
    ecmascript::AnFileDataManager::GetInstance()->SetDir("ark-profiler27/tmp");
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler27/");
    EcmaVM *ecmaVM = JSNApi::CreateJSVM(option);
    JSNApi::LoadAotFile(ecmaVM, "");
    EXPECT_TRUE(ecmaVM->IsEnablePGOProfiler());
    auto result = JSNApi::Execute(ecmaVM, targetAbcPath, targetRecordName, false);
    EXPECT_TRUE(result);
    JSNApi::DestroyJSVM(ecmaVM);
    EXPECT_TRUE(FileExist("ark-profiler27/modules.ap"));
    unlink("ark-profiler27/modules.ap");
    unlink("ark-profiler27/tmp.an");
    rmdir("ark-profiler27/");
}

HWTEST_F_L0(PGOProfilerTest, PGODisableWithAOTFileAppTest)
{
    mkdir("ark-profiler27/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::ofstream fWriter("ark-profiler27/tmp.an", std::fstream::app);
    fWriter.close();
    const char *targetRecordName = "typedarray_length";
    std::string targetAbcPath = std::string(TARGET_ABC_PATH) + targetRecordName + ".abc";
    // Force set in advance to simulate AOT/JIT enable list check is passed in UT
    ecmascript::AnFileDataManager::GetInstance()->SetEnable(true);
    ecmascript::AnFileDataManager::GetInstance()->SetDir("ark-profiler27/tmp");
    RuntimeOption option;
    EcmaVM *ecmaVM = JSNApi::CreateJSVM(option);
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler27/");
    JSNApi::PreFork(ecmaVM);
    JSNApi::PostFork(ecmaVM, option);
    EXPECT_TRUE(ecmaVM->IsEnablePGOProfiler());
    JSNApi::LoadAotFile(ecmaVM, "");
    EXPECT_FALSE(ecmaVM->IsEnablePGOProfiler());
    auto result = JSNApi::Execute(ecmaVM, targetAbcPath, targetRecordName, false);
    EXPECT_TRUE(result);
    JSNApi::DestroyJSVM(ecmaVM);
    EXPECT_FALSE(FileExist("ark-profiler27/modules.ap"));
    unlink("ark-profiler27/tmp.an");
    rmdir("ark-profiler27/");
}

HWTEST_F_L0(PGOProfilerTest, PGODisableUnderAOTFailTest)
{
    std::map<std::string, int32_t> mockAOTCompileStatusMap;
    mockAOTCompileStatusMap["module1"] = 0;
    mockAOTCompileStatusMap["module2"] = 1;
    mockAOTCompileStatusMap["module3"] = 4;
    RuntimeOption option;
    {
        // Not update existing setting when AOT compilation uninitialized or succeed
        EcmaVM *ecmaVM = JSNApi::CreateJSVM(option);
        RuntimeOption localOption = option;
        localOption.SetEnableProfile(true);
        localOption.SetAOTCompileStatusMap(mockAOTCompileStatusMap);
        JSNApi::PreFork(ecmaVM);
        JSNApi::PostFork(ecmaVM, localOption);
        EXPECT_FALSE(ecmaVM->GetJSOptions().GetAOTHasException());
        JSNApi::DestroyJSVM(ecmaVM);
    }
    {
        // Disable existing setting when AOT compilation failed
        EcmaVM *ecmaVM = JSNApi::CreateJSVM(option);
        RuntimeOption localOption = option;
        mockAOTCompileStatusMap["module4"] = 2;
        localOption.SetAOTCompileStatusMap(mockAOTCompileStatusMap);
        JSNApi::PreFork(ecmaVM);
        JSNApi::PostFork(ecmaVM, localOption);
        EXPECT_TRUE(ecmaVM->GetJSOptions().GetAOTHasException());
        JSNApi::DestroyJSVM(ecmaVM);
    }
    {
        // Disable existing setting when AOT compilation crashed
        EcmaVM *ecmaVM = JSNApi::CreateJSVM(option);
        RuntimeOption localOption = option;
        mockAOTCompileStatusMap["module4"] = 3;
        localOption.SetAOTCompileStatusMap(mockAOTCompileStatusMap);
        JSNApi::PreFork(ecmaVM);
        JSNApi::PostFork(ecmaVM, localOption);
        EXPECT_TRUE(ecmaVM->GetJSOptions().GetAOTHasException());
        JSNApi::DestroyJSVM(ecmaVM);
    }
}

HWTEST_F_L0(PGOProfilerTest, AnChecksumTest)
{
    AnFileDataManager *fileManager = AnFileDataManager::GetInstance();
    std::unordered_map<CString, uint32_t> testMap;
    testMap.emplace("test1.abc", 123456);
    fileManager->SafeMergeChecksumInfo(testMap);
    std::unordered_map<CString, uint32_t> fullMap;
    fullMap = fileManager->SafeGetfullFileNameToChecksumMap();
    ASSERT_FALSE(fullMap.empty());
    ASSERT_TRUE(fileManager->SafeCheckFilenameToChecksum("test1.abc", 123456));
    ASSERT_FALSE(fileManager->SafeCheckFilenameToChecksum("test1.abc", 654321));
    testMap.emplace("test2.abc", 456789);
    fileManager->SafeMergeChecksumInfo(testMap);
    fullMap = fileManager->SafeGetfullFileNameToChecksumMap();
    ASSERT_TRUE(fullMap.size() == 2);
    ASSERT_TRUE(fileManager->SafeCheckFilenameToChecksum("test2.abc", 456789));
    ASSERT_FALSE(fileManager->SafeCheckFilenameToChecksum("test2.abc", 987654));
    testMap["test1.abc"] = 123456789;
    fileManager->SafeMergeChecksumInfo(testMap);
    fullMap = fileManager->SafeGetfullFileNameToChecksumMap();
    ASSERT_TRUE(fullMap["test1.abc"] == static_cast<uint32_t>(-1));
    ASSERT_FALSE(fileManager->SafeCheckFilenameToChecksum("test1.abc", 123456789));
    ASSERT_TRUE(fileManager->SafeCheckFilenameToChecksum("test2.abc", 456789));
    fileManager->SafeDestroyAllData();
}

HWTEST_F_L0(PGOProfilerTest, PgoChecksumTest)
{
    mkdir("ark-pgoChecksumTest", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char *targetRecordName = "sample_test";
    ExecuteAndLoadJSPandaFile("ark-pgoChecksumTest/", targetRecordName);
    ASSERT_TRUE(FileExist("ark-pgoChecksumTest/modules.ap"));
    ASSERT_NE(pf_, nullptr);
    PGOProfilerManager::GetInstance()->Initialize("ark-pgoChecksumTest/modules.ap", DECODER_THRESHOLD);
    PGOProfilerDecoder decoder0("ark-pgoChecksumTest/modules.ap", DECODER_THRESHOLD);
    auto info = std::make_shared<PGOInfo>(DECODER_THRESHOLD);
    decoder0.LoadFull();
    info->MergeSafe(decoder0.GetPandaFileInfos());
    auto& infos1 = info->GetPandaFileInfos();
    auto infos1AbcFilePool = decoder0.GetAbcFilePool();
    CString testabc1Name = "test1.abc";
    uint32_t testabc1Checksum = 123456;
    ApEntityId abcId1(0);
    infos1AbcFilePool->TryAdd(testabc1Name, abcId1);
    std::unordered_map<CString, uint32_t> fileNameToChecksumMap1;
    fileNameToChecksumMap1.emplace(pf_->GetNormalizedFileDesc(), pf_->GetChecksum());
    fileNameToChecksumMap1.emplace(testabc1Name, testabc1Checksum);
    infos1.Sample(123456, abcId1);
    ASSERT_TRUE(infos1.Checksum(fileNameToChecksumMap1, infos1AbcFilePool));
    fileNameToChecksumMap1.emplace("notExist.abc", 456789);
    ASSERT_TRUE(infos1.Checksum(fileNameToChecksumMap1, infos1AbcFilePool));
    // different checksum
    fileNameToChecksumMap1[testabc1Name] = 999999;
    ASSERT_FALSE(infos1.Checksum(fileNameToChecksumMap1, infos1AbcFilePool));
    // pandaFileInfos always keep old checksum
    infos1.Sample(999999, abcId1);
    ASSERT_FALSE(infos1.Checksum(fileNameToChecksumMap1, infos1AbcFilePool));
    unlink("ark-pgoChecksumTest/modules.ap");
    unlink("ark-pgoChecksumTest/");
}
}  // namespace panda::test
