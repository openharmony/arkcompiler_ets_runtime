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

#include "ecmascript/elements.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_file_info.h"
#include "ecmascript/pgo_profiler/pgo_context.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/platform/file.h"
#include "gtest/gtest.h"

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/tests/pgo_context_mock.h"
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

    void TearDown() override
    {
        JSPandaFileManager::GetInstance()->RemoveJSPandaFileVm(vm_, pf_.get());
        vm_ = nullptr;
        pf_.reset();
        PGOProfilerManager::GetInstance()->Destroy();
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
        option.SetLogLevel(LOG_LEVEL::INFO);
        option.SetEnableProfile(true);
        option.SetProfileDir(profDir);
        vm_ = JSNApi::CreateJSVM(option);
        JSNApi::EnableUserUncaughtErrorHandler(vm_);

        std::string targetAbcPath = TARGET_ABC_PATH + recordName + ".abc";
        auto result = JSNApi::Execute(vm_, targetAbcPath, recordName, false);
        EXPECT_TRUE(result);

        pf_ = JSPandaFileManager::GetInstance()->FindJSPandaFile(CString(targetAbcPath));

        JSNApi::DestroyJSVM(vm_);
    }

    static constexpr uint32_t DECODER_THRESHOLD = 2;
    EcmaVM *vm_ = nullptr;
    std::shared_ptr<JSPandaFile> pf_;
};

HWTEST_F_L0(PGOProfilerTest, Sample)
{
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile("sample_test.abc", methodLiterals);

    mkdir("ark-profiler/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler/");
    vm_ = JSNApi::CreateJSVM(option);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "ark-profiler.abc");
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiterals[0]);
    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    method->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetPGOProfiler()->SetSaveTimestamp(std::chrono::system_clock::now());
    JSNApi::DestroyJSVM(vm_);
    // Loader
    PGOProfilerDecoder loader("ark-profiler/modules.ap", DECODER_THRESHOLD);
    CString expectRecordName = "sample_test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify(checksum));
    ASSERT_TRUE(!loader.Match(pf_.get(), expectRecordName, methodLiterals[0]->GetMethodId()));
#else
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
    ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName, methodLiterals[0]->GetMethodId()));
#endif
    unlink("ark-profiler/modules.ap");
    rmdir("ark-profiler/");
}

HWTEST_F_L0(PGOProfilerTest, Sample1)
{
    mkdir("ark-profiler1/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile("sample_test.abc", methodLiterals);

    mkdir("ark-profiler1/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler1/");
    vm_ = JSNApi::CreateJSVM(option);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "ark-profiler1.abc");
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiterals[0]);
    JSHandle<Method> method1 = vm_->GetFactory()->NewMethod(methodLiterals[1]);
    JSHandle<Method> method2 = vm_->GetFactory()->NewMethod(methodLiterals[2]);
    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    method1->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    method2->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());

    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    method->SetModule(vm_->GetJSThread(), recordName);
    method1->SetModule(vm_->GetJSThread(), recordName);
    method2->SetModule(vm_->GetJSThread(), recordName);
    JSNApi::DestroyJSVM(vm_);

    // Loader
    PGOProfilerDecoder loader("ark-profiler1/modules.ap", DECODER_THRESHOLD);
    CString expectRecordName = "sample_test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify(checksum));
    for (uint32_t idx = 0; idx < 3; idx++) {
        loader.MatchAndMarkMethod(pf_.get(), expectRecordName,
                                 methodLiterals[idx]->GetMethodName(pf_.get(), methodLiterals[idx]->GetMethodId()),
                                 methodLiterals[idx]->GetMethodId());
    }
    ASSERT_TRUE(!loader.Match(pf_.get(), expectRecordName, methodLiterals[0]->GetMethodId()));
    ASSERT_TRUE(!loader.Match(pf_.get(), expectRecordName, methodLiterals[2]->GetMethodId()));
    ASSERT_TRUE(!loader.Match(pf_.get(), expectRecordName, methodLiterals[1]->GetMethodId()));
#else
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
    ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName, methodLiterals[1]->GetMethodId()));
#endif
    unlink("ark-profiler1/modules.ap");
    rmdir("ark-profiler1/");
}

HWTEST_F_L0(PGOProfilerTest, Sample2)
{
    mkdir("ark-profiler2/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile("sample_test.abc", methodLiterals);

    mkdir("ark-profiler2/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler2/");
    vm_ = JSNApi::CreateJSVM(option);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "ark-profiler2.abc");

    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiterals[0]);
    JSHandle<Method> method1 = vm_->GetFactory()->NewMethod(methodLiterals[1]);

    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    method1->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    method->SetModule(vm_->GetJSThread(), recordName);
    JSHandle<JSTaggedValue> recordName1(vm_->GetFactory()->NewFromStdString("test1"));
    method1->SetModule(vm_->GetJSThread(), recordName1);
    JSNApi::DestroyJSVM(vm_);

    // Loader
    PGOProfilerDecoder loader("ark-profiler2/modules.ap", DECODER_THRESHOLD);
    CString expectRecordName = "sample_test";
    CString expectRecordName1 = "sample_test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify(checksum));
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
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
    ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName, methodLiterals[0]->GetMethodId()));
    ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName1, methodLiterals[1]->GetMethodId()));
#endif
    unlink("ark-profiler2/modules.ap");
    rmdir("ark-profiler2/");
}

HWTEST_F_L0(PGOProfilerTest, DisEnableSample)
{
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile("sample_test.abc", methodLiterals);
    EXPECT_GE(methodLiterals.size(), 1);
    mkdir("ark-profiler3/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(false);
    option.SetProfileDir("ark-profiler3/");
    vm_ = JSNApi::CreateJSVM(option);
    JSPandaFileManager::GetInstance()->AddJSPandaFileVm(vm_, pf_);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    uint32_t checksum = pf_->GetChecksum();
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "sample_test.abc");
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiterals[0]);

    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("sample_test"));
    method->SetModule(vm_->GetJSThread(), recordName);
    JSNApi::DestroyJSVM(vm_);

    // Loader
    ASSERT_FALSE(FileExist("ark-profiler3/modules.ap"));
    PGOProfilerDecoder loader("ark-profiler3/modules.ap", DECODER_THRESHOLD);
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
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
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "");
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    // RecordName is hole
    MethodLiteral *methodLiteral = new MethodLiteral(EntityId(61));
    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiteral);
    method->SetModule(vm_->GetJSThread(), JSTaggedValue::Hole());
    JSNApi::DestroyJSVM(vm_);

    PGOProfilerDecoder loader("", DECODER_THRESHOLD);
    // path is empty()
    ASSERT_TRUE(loader.LoadAndVerify(checksum));
    // path size greater than PATH_MAX
    char path[PATH_MAX + 1] = {'0'};
    PGOProfilerDecoder loader1(path, 4);
    ASSERT_TRUE(!loader1.LoadAndVerify(checksum));
}

HWTEST_F_L0(PGOProfilerTest, PGOProfilerDoubleVM)
{
    mkdir("ark-profiler5/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile("sample_test.abc", methodLiterals);
    EXPECT_GE(methodLiterals.size(), 2);  // number of methods
    mkdir("ark-profiler5/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    // outDir is empty
    option.SetProfileDir("ark-profiler5/");
    vm_ = JSNApi::CreateJSVM(option);
    JSPandaFileManager::GetInstance()->AddJSPandaFileVm(vm_, pf_);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "sample_test.abc");
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
    // worker vm read profile enable from PGOProfilerManager singleton
    option.SetEnableProfile(false);
    auto vm2 = JSNApi::CreateJSVM(option);
    JSHandle<ConstantPool> constPool2 = vm2->GetFactory()->NewConstantPool(4);
    constPool2->SetJSPandaFile(pf_.get());
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "sample_test.abc");
    ASSERT_TRUE(vm2 != nullptr) << "Cannot create Runtime";

    JSHandle<Method> method = vm2->GetFactory()->NewMethod(methodLiterals[0]);
    method->SetConstantPool(vm2->GetJSThread(), constPool2.GetTaggedValue());
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("sample_test"));
    method->SetModule(vm2->GetJSThread(), recordName);

    JSHandle<Method> method1 = vm_->GetFactory()->NewMethod(methodLiterals[0]);
    JSHandle<Method> method2 = vm_->GetFactory()->NewMethod(methodLiterals[1]);
    method1->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    method2->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSTaggedValue> recordName1(vm_->GetFactory()->NewFromStdString("sample_test"));
    method1->SetModule(vm_->GetJSThread(), recordName);
    method2->SetModule(vm_->GetJSThread(), recordName);

    JSNApi::DestroyJSVM(vm2);
    JSNApi::DestroyJSVM(vm_);

    PGOProfilerDecoder loader("ark-profiler5/profiler", DECODER_THRESHOLD);
    mkdir("ark-profiler5/profiler", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
    CString expectRecordName = "sample_test";
    ASSERT_TRUE(loader.Match(pf_.get(), expectRecordName, methodLiterals[1]->GetMethodId()));

    PGOProfilerDecoder loader1("ark-profiler5/modules.ap", DECODER_THRESHOLD);
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader1.LoadAndVerify(checksum));
    ASSERT_TRUE(!loader1.Match(pf_.get(), expectRecordName, methodLiterals[1]->GetMethodId()));
#else
    ASSERT_TRUE(!loader1.LoadAndVerify(checksum));
    ASSERT_TRUE(loader1.Match(pf_.get(), expectRecordName, methodLiterals[1]->GetMethodId()));
#endif

    unlink("ark-profiler5/modules.ap");
    rmdir("ark-profiler5/profiler");
    rmdir("ark-profiler5/");
}

HWTEST_F_L0(PGOProfilerTest, PGOProfilerDecoderNoHotMethod)
{
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile("sample_test.abc", methodLiterals);
    EXPECT_GE(methodLiterals.size(), 1);  // number of methods
    mkdir("ark-profiler8/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler8/");
    vm_ = JSNApi::CreateJSVM(option);
    JSPandaFileManager::GetInstance()->AddJSPandaFileVm(vm_, pf_);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    uint32_t checksum = pf_->GetChecksum();
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "sample_test.abc");

    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiterals[0]);

    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("sample_test"));
    method->SetModule(vm_->GetJSThread(), recordName);
    JSNApi::DestroyJSVM(vm_);

    PGOProfilerDecoder loader("ark-profiler8/modules.ap", DECODER_THRESHOLD);
    CString expectRecordName = "sample_test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify(checksum));
    ASSERT_TRUE(!loader.Match(pf_.get(), expectRecordName, methodLiterals[0]->GetMethodId()));
#else
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
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
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile(sourceStream.str().c_str(), "ark-profiler9.abc", methodLiterals);
    EXPECT_EQ(methodLiterals.size(), 100);  // number of methods
    mkdir("ark-profiler9/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler9/");
    vm_ = JSNApi::CreateJSVM(option);
    JSPandaFileManager::GetInstance()->AddJSPandaFileVm(vm_, pf_);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SetApGenMode(ApGenMode::OVERWRITE);
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "ark-profiler9.abc");

    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("ark-profiler9.abc"));
    for (int i = 61; i < 91; i++) {
        JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiterals[i]);
        method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
        method->SetModule(vm_->GetJSThread(), recordName);
    }

    JSNApi::DestroyJSVM(vm_);

    PGOProfilerDecoder loader("ark-profiler9/modules.ap", DECODER_THRESHOLD);
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify(checksum));
#else
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
#endif
    CString expectRecordName = "ark-profiler9.abc";
    for (int i = 0; i < 100; i++) {
        EntityId methodId = methodLiterals[i]->GetMethodId();
        loader.MatchAndMarkMethod(pf_.get(), expectRecordName,
                                  methodLiterals[i]->GetMethodName(pf_.get(), methodId), methodId);
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

HWTEST_F_L0(PGOProfilerTest, BinaryToText)
{
    mkdir("ark-profiler7/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    std::fstream file("ark-profiler7/modules.ap",
                      std::fstream::binary | std::fstream::out | std::fstream::in | std::fstream::trunc);

    PGOProfilerHeader *header = nullptr;
    PGOProfilerHeader::Build(&header, PGOProfilerHeader::LastSize());
    std::unique_ptr<PGOAbcFilePool> abcFilePool = std::make_unique<PGOAbcFilePool>();
    std::unique_ptr<PGOPandaFileInfos> pandaFileInfos = std::make_unique<PGOPandaFileInfos>();
    std::unique_ptr<PGORecordDetailInfos> recordInfos = std::make_unique<PGORecordDetailInfos>(2);

    RuntimeOption option;
    vm_ = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
    pandaFileInfos->Sample(0x34556738);
    std::shared_ptr<MethodLiteral> methodLiteral = std::make_shared<MethodLiteral>(EntityId(61));
    auto *jsMethod =
        Method::Cast(vm_->GetFactory()->NewMethod(methodLiteral.get(), MemSpaceType::NON_MOVABLE).GetTaggedValue());

    ApEntityId recordId(0);
    recordInfos->GetRecordPool()->TryAdd("test", recordId);
    ProfileType recordType(0, recordId, ProfileType::Kind::LocalRecordId);
    ASSERT_TRUE(recordInfos->AddMethod(recordType, jsMethod, SampleMode::CALL_MODE));
    ASSERT_FALSE(recordInfos->AddMethod(recordType, jsMethod, SampleMode::CALL_MODE));
    ASSERT_FALSE(recordInfos->AddMethod(recordType, jsMethod, SampleMode::CALL_MODE));

    pandaFileInfos->ProcessToBinary(file, header->GetPandaInfoSection());
    recordInfos->ProcessToBinary(nullptr, file, header);
    PGOFileSectionInterface::ProcessSectionToBinary(file, header, *abcFilePool->GetPool());
    header->SetFileSize(static_cast<uint32_t>(file.tellp()));
    header->ProcessToBinary(file);
    PGOProfilerEncoder::AddChecksum(file);
    file.close();

    ASSERT_TRUE(PGOProfilerManager::GetInstance()->BinaryToText(
        "ark-profiler7/modules.ap", "ark-profiler7/modules.text", 2));
    JSNApi::DestroyJSVM(vm_);
    unlink("ark-profiler7/modules.ap");
    unlink("ark-profiler7/modules.text");
    rmdir("ark-profiler7");
}

HWTEST_F_L0(PGOProfilerTest, TextToBinary)
{
    mkdir("ark-profiler10/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    std::ofstream file("ark-profiler10/modules.text");
    std::string result = "Profiler Version: 0.0.0.1\n";
    file.write(result.c_str(), result.size());
    result = "\nPanda file sumcheck list: [ 413775942 ]\n";
    file.write(result.c_str(), result.size());
    result = "\nrecordName: [ 1232/3/CALL_MODE/hello, 234/100/HOTNESS_MODE/h#ello1 ]\n";
    file.write(result.c_str(), result.size());
    file.close();

    ASSERT_TRUE(PGOProfilerManager::GetInstance()->TextToBinary("ark-profiler10/modules.text", "ark-profiler10/", 2,
                                                                ApGenMode::OVERWRITE));

    PGOProfilerDecoder loader("ark-profiler10/modules.ap", DECODER_THRESHOLD);
    ASSERT_TRUE(loader.LoadAndVerify(413775942));

    unlink("ark-profiler10/modules.ap");
    unlink("ark-profiler10/modules.text");
    rmdir("ark-profiler10");
}

HWTEST_F_L0(PGOProfilerTest, FailResetProfilerInWorker)
{
    std::vector<MethodLiteral *> methodLiterals {};
    CreateJSPandaFile("sample_test.abc", methodLiterals);
    EXPECT_GE(methodLiterals.size(), 1);  // number of methods
    mkdir("ark-profiler12/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    // Although enableProfile is set in option, but it will not work when isWorker is set.
    option.SetEnableProfile(true);
    option.SetIsWorker();
    option.SetProfileDir("ark-profiler12/");
    // PgoProfiler is disabled as default.
    vm_ = JSNApi::CreateJSVM(option);
    JSPandaFileManager::GetInstance()->AddJSPandaFileVm(vm_, pf_);
    uint32_t checksum = pf_->GetChecksum();
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum, "sample_test.abc");
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiterals[0]);

    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf_.get());
    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("sample_test"));
    method->SetModule(vm_->GetJSThread(), recordName);
    JSNApi::DestroyJSVM(vm_);

    // Loader
    ASSERT_FALSE(FileExist("ark-profiler12/modules.ap"));
    PGOProfilerDecoder loader("ark-profiler12/modules.ap", DECODER_THRESHOLD);
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
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

    // Loader
    PGOProfilerDecoder decoder("ark-profiler13/modules.ap", 1);
    PGOProfilerDecoder decoder1("ark-profiler13/modules.ap", 10);
    PGOProfilerDecoder decoder2("ark-profiler13/modules.ap", 11000);
    ASSERT_TRUE(decoder.LoadAndVerify(checksum));
    ASSERT_TRUE(decoder1.LoadAndVerify(checksum));
    ASSERT_TRUE(decoder2.LoadAndVerify(checksum));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        decoder1.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        decoder2.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        ASSERT_TRUE(decoder.Match(pf_.get(), targetRecordName, methodId));
        if (std::string(methodName) == "Test") {
            ASSERT_TRUE(decoder1.Match(pf_.get(), targetRecordName, methodId));
            ASSERT_TRUE(decoder2.Match(pf_.get(), targetRecordName, methodId));
        } else {
            ASSERT_TRUE(decoder1.Match(pf_.get(), targetRecordName, methodId));
            ASSERT_TRUE(decoder2.Match(pf_.get(), targetRecordName, methodId));
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
    ASSERT_TRUE(decoder.LoadAndVerify(checksum));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        ASSERT_TRUE(decoder.Match(pf_.get(), targetRecordName, methodId));
        auto callback = [methodName, methodId](uint32_t offset, PGOType *type) {
            ASSERT_NE(offset, 0);
            if (type->IsScalarOpType()) {
            } else if (type->IsRwOpType()) {
                auto pgoRWOpType = *reinterpret_cast<PGORWOpType *>(type);
                if (std::string(methodName) == "Foot" || std::string(methodName) == "Arm") {
                    ASSERT_TRUE(pgoRWOpType.GetCount() == 1);
                    ASSERT_EQ(pgoRWOpType.GetObjectInfo(0).GetProfileType(),
                              ProfileType(ApEntityId(PGORecordPool::RESERVED_COUNT), methodId.GetOffset()));
                } else if (std::string(methodName) == "foo" || std::string(methodName) == "Body") {
                    ASSERT_TRUE(pgoRWOpType.GetCount() == 3);
                }
            } else {
                ASSERT_TRUE(false);
            }
        };
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral,
                            callback);
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
    ASSERT_TRUE(decoder.LoadAndVerify(checksum));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        ASSERT_TRUE(decoder.Match(pf_.get(), targetRecordName, methodId));
        auto callback = [methodName, &decoder, jsPandaFile = pf_](uint32_t offset, PGOType *type) {
            ASSERT_NE(offset, 0);
            if (type->IsScalarOpType()) {
                auto sampleType = *reinterpret_cast<PGOSampleType *>(type);
                if (sampleType.IsProfileType()) {
                    ASSERT_EQ(std::string(methodName), "func_main_0");
                    PGOHClassLayoutDesc *desc;
                    if (!decoder.GetHClassLayoutDesc(sampleType, &desc)) {
                        return;
                    }
                    ASSERT_EQ(desc->GetCtorLayoutDesc().size(), 3);
                    ASSERT_EQ(desc->GetPtLayoutDesc().size(), 1);
                    auto classId = EntityId(sampleType.GetProfileType().GetId());
                    auto className = MethodLiteral::GetMethodName(jsPandaFile.get(), classId);
                    if (std::string(className) == "Arm") {
                        auto superClassId = EntityId(desc->GetSuperProfileType().GetId());
                        auto superClassName = MethodLiteral::GetMethodName(jsPandaFile.get(), superClassId);
                        ASSERT_EQ(std::string(superClassName), "Body");
                        ASSERT_EQ(desc->GetLayoutDesc().size(), 0);
                    } else if (std::string(className) == "Foot") {
                        auto superClassId = EntityId(desc->GetSuperProfileType().GetId());
                        auto superClassName = MethodLiteral::GetMethodName(jsPandaFile.get(), superClassId);
                        ASSERT_EQ(std::string(superClassName), "Body");
                        ASSERT_EQ(desc->GetLayoutDesc().size(), 0);
                    } else {
                        ASSERT_EQ(desc->GetSuperProfileType().GetRaw(), 0);
                        ASSERT_EQ(desc->GetLayoutDesc().size(), 0);
                    }
                }
            }
        };
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral,
                            callback);
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
    ASSERT_TRUE(decoder.LoadAndVerify(checksum));
    std::string types[17] =
        { "1", "5", "4", "4", "4", "4", "4", "4", "5", "4", "4", "1", "4", "5", "1" };
    int index = 0;
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        if (std::string(methodName) != "sun" && std::string(methodName) != "sun1") {
            decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
            ASSERT_TRUE(decoder.Match(pf_.get(), targetRecordName, methodId));
        }
        auto callback = [methodName, types, &index](uint32_t offset, PGOType *type) {
            ASSERT_NE(offset, 0);
            if (type->IsScalarOpType()) {
                auto sampleType = *reinterpret_cast<PGOSampleType *>(type);
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
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral,
                            callback);
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
    ASSERT_TRUE(decoder.LoadAndVerify(checksum));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        ASSERT_TRUE(decoder.Match(pf_.get(), targetRecordName, methodId));
        auto callback = [methodName, &decoder, jsPandaFile = pf_](uint32_t offset, PGOType *type) {
            if (type->IsScalarOpType()) {
                auto sampleType = *reinterpret_cast<PGOSampleType *>(type);
                if (sampleType.IsProfileType()) {
                    ASSERT_EQ(std::string(methodName), "func_main_0");
                    PGOHClassLayoutDesc *desc;
                    if (!decoder.GetHClassLayoutDesc(sampleType, &desc)) {
                        return;
                    }
                    ASSERT_EQ(desc->GetCtorLayoutDesc().size(), 0);
                    ASSERT_EQ(desc->GetPtLayoutDesc().size(), 0);
                    ASSERT_EQ(desc->GetLayoutDesc().size(), 1);
                }
            } else if (type->IsRwOpType()) {
                auto pgoRWOpType = *reinterpret_cast<PGORWOpType *>(type);
                if (std::string(methodName) == "foo") {
                    ASSERT_TRUE(pgoRWOpType.GetCount() == 3);
                    auto classType = pgoRWOpType.GetObjectInfo(0).GetProfileType();
                    ASSERT_TRUE(classType.IsElementType());
                    ASSERT_EQ(classType.GetId(), static_cast<uint32_t>(ElementsKind::NUMBER));

                    classType = pgoRWOpType.GetObjectInfo(1).GetProfileType();
                    ASSERT_TRUE(classType.IsElementType());
                    ASSERT_EQ(classType.GetId(), static_cast<uint32_t>(ElementsKind::HOLE_INT));

                    classType = pgoRWOpType.GetObjectInfo(2).GetProfileType();
                    ASSERT_TRUE(classType.IsElementType());
                    ASSERT_EQ(classType.GetId(), static_cast<uint32_t>(ElementsKind::TAGGED));
                } else if (std::string(methodName) == "foo1") {
                    ASSERT_TRUE(pgoRWOpType.GetCount() == 1);
                    auto classType = pgoRWOpType.GetObjectInfo(0).GetProfileType();
                    ASSERT_TRUE(classType.IsElementType());
                    ASSERT_EQ(classType.GetId(), static_cast<uint32_t>(ElementsKind::TAGGED));
                }
            }
        };
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral,
                            callback);
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
    ASSERT_TRUE(decoder.LoadAndVerify(checksum));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        ASSERT_TRUE(decoder.Match(pf_.get(), targetRecordName, methodId));
        auto callback = [methodName, &decoder, jsPandaFile = pf_](uint32_t offset, PGOType *type) {
            if (type->IsScalarOpType()) {
                auto sampleType = *reinterpret_cast<PGOSampleType *>(type);
                if (sampleType.IsProfileType()) {
                    ASSERT_EQ(std::string(methodName), "func_main_0");
                    PGOHClassLayoutDesc *desc;
                    if (!decoder.GetHClassLayoutDesc(sampleType, &desc)) {
                        return;
                    }
                    ASSERT_EQ(desc->GetCtorLayoutDesc().size(), 0);
                    ASSERT_EQ(desc->GetPtLayoutDesc().size(), 0);
                    ASSERT_EQ(desc->GetLayoutDesc().size(), 3);
                }
            } else if (type->IsRwOpType()) {
                auto pgoRWOpType = *reinterpret_cast<PGORWOpType *>(type);
                if (std::string(methodName) == "foo") {
                    ASSERT_TRUE(pgoRWOpType.GetCount() == 2);
                    auto classType = PGOSampleType(pgoRWOpType.GetObjectInfo(0).GetProfileType());
                    PGOHClassLayoutDesc *desc;
                    ASSERT_TRUE(decoder.GetHClassLayoutDesc(classType, &desc));
                    ASSERT_EQ(desc->GetLayoutDesc()[0].first, "x");
                    ASSERT_EQ(desc->GetLayoutDesc()[0].second.GetTrackType(), TrackType::INT);
                    ASSERT_EQ(desc->GetLayoutDesc()[1].first, "y");
                    ASSERT_EQ(desc->GetLayoutDesc()[1].second.GetTrackType(), TrackType::INT);
                    ASSERT_EQ(desc->GetLayoutDesc()[2].first, "z");
                    ASSERT_EQ(desc->GetLayoutDesc()[2].second.GetTrackType(), TrackType::INT);

                    classType = PGOSampleType(pgoRWOpType.GetObjectInfo(1).GetProfileType());
                    ASSERT_TRUE(decoder.GetHClassLayoutDesc(classType, &desc));
                    ASSERT_EQ(desc->GetLayoutDesc()[0].first, "u");
                    ASSERT_EQ(desc->GetLayoutDesc()[0].second.GetTrackType(), TrackType::DOUBLE);
                    ASSERT_EQ(desc->GetLayoutDesc()[1].first, "y");
                    ASSERT_EQ(desc->GetLayoutDesc()[1].second.GetTrackType(), TrackType::NUMBER);
                    ASSERT_EQ(desc->GetLayoutDesc()[2].first, "t");
                    ASSERT_EQ(desc->GetLayoutDesc()[2].second.GetTrackType(), TrackType::TAGGED);
                }
            }
        };
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral,
                            callback);
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
    ASSERT_TRUE(decoder.LoadAndVerify(checksum));
    auto methodLiterals = pf_->GetMethodLiteralMap();
    for (auto iter : methodLiterals) {
        auto methodLiteral = iter.second;
        auto methodId = methodLiteral->GetMethodId();
        auto methodName = methodLiteral->GetMethodName(pf_.get(), methodId);
        decoder.MatchAndMarkMethod(pf_.get(), targetRecordName, methodName, methodId);
        ASSERT_TRUE(decoder.Match(pf_.get(), targetRecordName, methodId));
        auto callback = [methodName, &decoder, jsPandaFile = pf_](uint32_t offset, PGOType *type) {
            if (type->IsScalarOpType()) {
                auto sampleType = *reinterpret_cast<PGOSampleType *>(type);
                if (sampleType.IsProfileType()) {
                    PGOHClassLayoutDesc *desc;
                    if (!decoder.GetHClassLayoutDesc(sampleType, &desc)) {
                        return;
                    }
                    if (std::string(methodName) == "foo") {
                        ASSERT_EQ(desc->GetArrayLength(), 4);
                    } else if (std::string(methodName) == "foo1") {
                        ASSERT_EQ(desc->GetArrayLength(), 8);
                    } else if (std::string(methodName) == "foo2") {
                        ASSERT_EQ(desc->GetArrayLength(), 8);
                    }
                }
            }
        };
        decoder.GetTypeInfo(pf_.get(), targetRecordName, methodLiteral,
                            callback);
    }
    unlink("ark-profiler21/modules.ap");
    rmdir("ark-profiler21/");
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
    ASSERT_FALSE(loader.LoadAndVerify(checksum));
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

HWTEST_F_L0(PGOProfilerTest, ClassTypeLegacyCheckForWideClassType)
{
    PGOContextMock context(PGOProfilerHeader::RECORD_POOL_MINI_VERSION);
    ProfileTypeLegacy classTypeLegacy(0xafe, ProfileType::Kind::ElementId);
    auto &profileTypeRef = *(static_cast<ProfileTypeRef *>(static_cast<void *>(&classTypeLegacy)));
    ProfileType classType(context, profileTypeRef);
    ASSERT_EQ(classTypeLegacy.GetId(), 0xafe);
    ASSERT_EQ(classTypeLegacy.GetKind(), ProfileType::Kind::ElementId);
    ASSERT_EQ(classType.GetId(), 0xafe);
    ASSERT_EQ(classType.GetKind(), ProfileType::Kind::ElementId);
}

HWTEST_F_L0(PGOProfilerTest, PGOSampleTypeLegacyCheckForWideClassType)
{
    PGOContextMock context(PGOProfilerHeader::RECORD_POOL_MINI_VERSION);
    PGOSampleTypeRef sampleTypeLegacyType(PGOSampleTypeRef::Type::NUMBER);
    PGOSampleType sampleType = PGOSampleType::ConvertFrom(context, sampleTypeLegacyType);
    ASSERT_TRUE(sampleTypeLegacyType.IsNumber());
    ASSERT_TRUE(sampleType.IsNumber());
    ASSERT_FALSE(sampleType.IsProfileType());

    ProfileTypeLegacy classTypeLegacy(0xafe, ProfileType::Kind::ElementId);
    auto &profileTypeRef = *(static_cast<ProfileTypeRef *>(static_cast<void *>(&classTypeLegacy)));
    PGOSampleTypeRef sampleTypeLegacyClass(profileTypeRef);
    ASSERT_TRUE(sampleTypeLegacyClass.IsProfileType());

    sampleType = PGOSampleType::ConvertFrom(context, sampleTypeLegacyClass);
    ASSERT_FALSE(sampleType.IsNumber());
    ASSERT_TRUE(sampleType.IsProfileType());
    ASSERT_EQ(sampleType.GetProfileType().GetId(), classTypeLegacy.GetId());
    ASSERT_EQ(sampleType.GetProfileType().GetKind(), classTypeLegacy.GetKind());
}

HWTEST_F_L0(PGOProfilerTest, PGOObjectInfoLegacyCheckForWideClassType)
{
    PGOContextMock context(PGOProfilerHeader::RECORD_POOL_MINI_VERSION);
    ProfileTypeLegacy classTypeLegacy(0xafe, ProfileType::Kind::ElementId);
    auto &profileTypeRef = *(static_cast<ProfileTypeRef *>(static_cast<void *>(&classTypeLegacy)));
    PGOObjectInfoRef objInfoLegacy(profileTypeRef, PGOObjKind::CONSTRUCTOR);
    PGOObjectInfo objInfo;
    objInfo.ConvertFrom(context, objInfoLegacy);
    ASSERT_EQ(objInfo.GetObjKind(), objInfoLegacy.GetObjKind());
    ASSERT_EQ(objInfo.GetProfileType().GetId(), 0xafe);
    ASSERT_EQ(objInfo.GetProfileType().GetKind(), ProfileType::Kind::ElementId);
}

HWTEST_F_L0(PGOProfilerTest, PGORWOpTypeLegacyCheckForWideClassType)
{
    PGOContextMock context(PGOProfilerHeader::RECORD_POOL_MINI_VERSION);
    PGORWOpTypeRef rwOpLegacy;
    // add item1
    ProfileTypeLegacy classTypeLegacy1(0xafe, ProfileType::Kind::ElementId);
    auto &profileTypeRef1 = *(static_cast<ProfileTypeRef *>(static_cast<void *>(&classTypeLegacy1)));
    PGOObjectInfoRef infoLegacy1(profileTypeRef1, PGOObjKind::CONSTRUCTOR);
    rwOpLegacy.AddObjectInfo(infoLegacy1);

    // add item2
    ProfileTypeLegacy classTypeLegacy2(0xaff, ProfileType::Kind::BuiltinsId);
    auto &profileTypeRef2 = *(static_cast<ProfileTypeRef *>(static_cast<void *>(&classTypeLegacy2)));
    PGOObjectInfoRef infoLegacy2(profileTypeRef2, PGOObjKind::ELEMENT);
    rwOpLegacy.AddObjectInfo(infoLegacy2);

    PGORWOpType rwOp;
    rwOp.ConvertFrom(context, rwOpLegacy);
    ASSERT_EQ(rwOp.GetCount(), rwOpLegacy.GetCount());
    // get item
    PGOObjectInfo info1 = rwOp.GetObjectInfo(0);
    PGOObjectInfo info2 = rwOp.GetObjectInfo(1);

    ASSERT_EQ(info1.GetObjKind(), PGOObjKind::CONSTRUCTOR);
    ASSERT_EQ(info2.GetProfileType().GetId(), 0xaff);
    ASSERT_EQ(info2.GetProfileType().GetKind(), ProfileType::Kind::BuiltinsId);
}

HWTEST_F_L0(PGOProfilerTest, PGOHClassLayoutDescInnerLegacyCheckForWideClassType)
{
    PGOContextMock context(PGOProfilerHeader::RECORD_POOL_MINI_VERSION);
    // create legacy
    ProfileTypeLegacy classTypeLegacy(0xafe, ProfileType::Kind::ClassId);
    auto &profileTypeRef = *(static_cast<ProfileTypeRef *>(static_cast<void *>(&classTypeLegacy)));
    PGOSampleTypeRef sampleTypeLegacyClass(profileTypeRef);

    ProfileTypeLegacy superClassTypeLegacy(0xaff, ProfileType::Kind::ElementId);
    auto &superProfileTypeRef = *(static_cast<ProfileTypeRef *>(static_cast<void *>(&superClassTypeLegacy)));
    PGOSampleTypeRef superSampleTypeLegacyClass(superProfileTypeRef);
    auto elementsKind = ElementsKind::HOLE_NUMBER;
    PGOHClassLayoutDesc desc;
    desc.SetElementsKind(elementsKind);
    ElementsTrackInfo trackInfo;
    size_t size = PGOHClassLayoutDescInnerRef::CaculateSize(desc);
    PGOHClassLayoutDescInnerRef layoutLegacy(size, sampleTypeLegacyClass, superSampleTypeLegacyClass, elementsKind,
                                             trackInfo);

    PGOHClassLayoutDesc descRecover = layoutLegacy.Convert(context);

    ASSERT_EQ(descRecover.GetElementsKind(), elementsKind);
    ASSERT_EQ(descRecover.GetProfileType().GetId(), 0xafe);
    ASSERT_EQ(descRecover.GetProfileType().GetKind(), ProfileType::Kind::ClassId);
    ASSERT_EQ(descRecover.GetSuperProfileType().GetId(), 0xaff);
    ASSERT_EQ(descRecover.GetSuperProfileType().GetKind(), ProfileType::Kind::ElementId);
}
}  // namespace panda::test
