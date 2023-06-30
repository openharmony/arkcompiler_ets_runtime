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
#include <memory>
#include <sstream>
#include <string>
#include "ecmascript/object_factory.h"
#include "gtest/gtest.h"

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/jspandafile/program_object.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
class PGOProfilerTest : public testing::Test {
public:
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
        vm_ = nullptr;
        PGOProfilerManager::GetInstance()->Destroy();
    }

protected:
    std::shared_ptr<JSPandaFile> CreateJSPandaFile(const char *source, const CString filename,
                                                   std::vector<MethodLiteral *> &methodLiterals)
    {
        Parser parser;
        const std::string fn = "SRC.abc";  // test file name : "SRC.abc"
        auto res = parser.Parse(source, fn);

        std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
        JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
        std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), filename);

        const File *file = pf->GetPandaFile();
        const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
        File::EntityId classId = file->GetClassId(typeDesc);
        EXPECT_TRUE(classId.IsValid());

        ClassDataAccessor cda(*file, classId);
        cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
            auto *methodLiteral = new MethodLiteral(mda.GetMethodId());
            methodLiteral->Initialize(pf.get());
            pf->SetMethodLiteralToMap(methodLiteral);
            methodLiterals.push_back(methodLiteral);
        });
        return pf;
    }

    EcmaVM *vm_ = nullptr;
};

HWTEST_F_L0(PGOProfilerTest, Sample)
{
    const char *source = R"(
        .language ECMAScript
        .function void foo1(any a0, any a1, any a2) {}
    )";
    std::vector<MethodLiteral *> methodLiterals {};
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, "ark-profiler.abc", methodLiterals);
    EXPECT_EQ(methodLiterals.size(), 1);  // number of methods

    mkdir("ark-profiler/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler/");
    vm_ = JSNApi::CreateJSVM(option);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf.get());
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiterals[0]);
    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetPGOProfiler()->SetSaveTimestamp(std::chrono::system_clock::now());
    vm_->GetPGOProfiler()->ProfileCall(func.GetTaggedType());
    JSNApi::DestroyJSVM(vm_);
    // Loader
    PGOProfilerDecoder loader("ark-profiler/modules.ap", 2);
    CString expectRecordName = "test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify(checksum));
    ASSERT_TRUE(!loader.Match(expectRecordName, methodLiterals[0]->GetMethodId()));
#else
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
    ASSERT_TRUE(loader.Match(expectRecordName, methodLiterals[0]->GetMethodId()));
#endif
    unlink("ark-profiler/modules.ap");
    rmdir("ark-profiler/");
}

HWTEST_F_L0(PGOProfilerTest, Sample1)
{
    const char *source = R"(
        .language ECMAScript
        .function void foo1(any a0, any a1, any a2) {
            lda.str "helloworld"
            return
        }
        .function void foo2(any a0, any a1, any a2) {}
        .function void foo3(any a0, any a1, any a2) {}
    )";
    std::vector<MethodLiteral *> methodLiterals {};
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, "ark-profiler1.abc", methodLiterals);
    EXPECT_EQ(methodLiterals.size(), 3);  // number of methods

    mkdir("ark-profiler1/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler1/");
    vm_ = JSNApi::CreateJSVM(option);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf.get());
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiterals[0]);
    JSHandle<Method> method1 = vm_->GetFactory()->NewMethod(methodLiterals[1]);
    JSHandle<Method> method2 = vm_->GetFactory()->NewMethod(methodLiterals[2]);
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
    for (int i = 0; i < 5; i++) {
        vm_->GetPGOProfiler()->ProfileCall(func.GetTaggedType());
    }
    for (int i = 0; i < 50; i++) {
        vm_->GetPGOProfiler()->ProfileCall(func2.GetTaggedType());
    }
    vm_->GetPGOProfiler()->ProfileCall(func1.GetTaggedType());
    JSNApi::DestroyJSVM(vm_);

    // Loader
    PGOProfilerDecoder loader("ark-profiler1/modules.ap", 2);
    CString expectRecordName = "test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify(checksum));
    for (uint32_t idx = 0; idx < 3; idx++) {
        loader.MatchAndMarkMethod(expectRecordName,
                                 methodLiterals[idx]->GetMethodName(pf.get(), methodLiterals[idx]->GetMethodId()),
                                 methodLiterals[idx]->GetMethodId());
    }
    ASSERT_TRUE(loader.Match(expectRecordName, methodLiterals[0]->GetMethodId()));
    ASSERT_TRUE(loader.Match(expectRecordName, methodLiterals[2]->GetMethodId()));
    ASSERT_TRUE(!loader.Match(expectRecordName, methodLiterals[1]->GetMethodId()));
#else
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
    ASSERT_TRUE(loader.Match(expectRecordName, methodLiterals[1]->GetMethodId()));
#endif
    unlink("ark-profiler1/modules.ap");
    rmdir("ark-profiler1/");
}

HWTEST_F_L0(PGOProfilerTest, Sample2)
{
    const char *source = R"(
        .language ECMAScript
        .function void foo1(any a0, any a1, any a2) {}
        .function void foo2(any a0, any a1, any a2) {}
    )";
    std::vector<MethodLiteral *> methodLiterals {};
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, "ark-profiler2.abc", methodLiterals);
    EXPECT_EQ(methodLiterals.size(), 2);  // number of methods

    mkdir("ark-profiler2/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler2/");
    vm_ = JSNApi::CreateJSVM(option);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf.get());
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);

    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiterals[0]);
    JSHandle<Method> method1 = vm_->GetFactory()->NewMethod(methodLiterals[1]);

    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    method1->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    JSHandle<JSFunction> func1 = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method1);
    JSHandle<JSTaggedValue> recordName1(vm_->GetFactory()->NewFromStdString("test1"));
    func1->SetModule(vm_->GetJSThread(), recordName1);
    vm_->GetPGOProfiler()->ProfileCall(func.GetTaggedType());
    for (int i = 0; i < 5; i++) {
        vm_->GetPGOProfiler()->ProfileCall(func1.GetTaggedType());
    }
    JSNApi::DestroyJSVM(vm_);

    // Loader
    PGOProfilerDecoder loader("ark-profiler2/modules.ap", 2);
    CString expectRecordName = "test";
    CString expectRecordName1 = "test1";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify(checksum));
    for (uint32_t idx = 0; idx < 2; idx++) {
        loader.MatchAndMarkMethod(expectRecordName,
                                 methodLiterals[idx]->GetMethodName(pf.get(), methodLiterals[idx]->GetMethodId()),
                                 methodLiterals[idx]->GetMethodId());
        loader.MatchAndMarkMethod(expectRecordName1,
                                 methodLiterals[idx]->GetMethodName(pf.get(), methodLiterals[idx]->GetMethodId()),
                                 methodLiterals[idx]->GetMethodId());
    }
    ASSERT_TRUE(!loader.Match(expectRecordName, methodLiterals[0]->GetMethodId()));
#else
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
    ASSERT_TRUE(loader.Match(expectRecordName, methodLiterals[0]->GetMethodId()));
#endif
    ASSERT_TRUE(loader.Match(expectRecordName1, methodLiterals[1]->GetMethodId()));
    unlink("ark-profiler2/modules.ap");
    rmdir("ark-profiler2/");
}

HWTEST_F_L0(PGOProfilerTest, DisEnableSample)
{
    const char *source = R"(
        .language ECMAScript
        .function void foo1(any a0, any a1, any a2) {}
    )";
    std::vector<MethodLiteral *> methodLiterals {};
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, "ark-profiler3.abc", methodLiterals);
    EXPECT_EQ(methodLiterals.size(), 1);  // number of methods
    mkdir("ark-profiler3/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(false);
    option.SetProfileDir("ark-profiler3/");
    vm_ = JSNApi::CreateJSVM(option);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf.get());
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiterals[0]);

    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetPGOProfiler()->ProfileCall(func.GetTaggedType());
    JSNApi::DestroyJSVM(vm_);

    // Loader
    PGOProfilerDecoder loader("ark-profiler3/modules.ap", 2);
    // path is empty()
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
    CString expectRecordName = "test";
    ASSERT_TRUE(loader.Match(expectRecordName, methodLiterals[0]->GetMethodId()));
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
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSHandle<JSArray> array = vm_->GetFactory()->NewJSArray();
    vm_->GetPGOProfiler()->ProfileCall(array.GetTaggedType());

    // RecordName is hole
    MethodLiteral *methodLiteral = new MethodLiteral(EntityId(61));
    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiteral);
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    func->SetModule(vm_->GetJSThread(), JSTaggedValue::Hole());
    vm_->GetPGOProfiler()->ProfileCall(func.GetTaggedType());
    JSNApi::DestroyJSVM(vm_);

    PGOProfilerDecoder loader("", 2);
    // path is empty()
    ASSERT_TRUE(loader.LoadAndVerify(checksum));
    // path size greater than PATH_MAX
    char path[PATH_MAX + 1] = {'0'};
    PGOProfilerDecoder loader1(path, 4);
    ASSERT_TRUE(!loader1.LoadAndVerify(checksum));
}

HWTEST_F_L0(PGOProfilerTest, PGOProfilerDoubleVM)
{
    const char *source = R"(
        .language ECMAScript
        .function void foo1(any a0, any a1, any a2) {}
        .function void foo2(any a0, any a1, any a2) {}
    )";
    std::vector<MethodLiteral *> methodLiterals {};
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, "ark-profiler5.abc", methodLiterals);
    EXPECT_EQ(methodLiterals.size(), 2);  // number of methods
    mkdir("ark-profiler5/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    // outDir is empty
    option.SetProfileDir("ark-profiler5/");
    vm_ = JSNApi::CreateJSVM(option);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf.get());
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
    // worker vm read profile enable from PGOProfilerManager singleton
    option.SetEnableProfile(false);
    auto vm2 = JSNApi::CreateJSVM(option);
    JSHandle<ConstantPool> constPool2 = vm2->GetFactory()->NewConstantPool(4);
    constPool2->SetJSPandaFile(pf.get());
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);
    ASSERT_TRUE(vm2 != nullptr) << "Cannot create Runtime";

    JSHandle<Method> method = vm2->GetFactory()->NewMethod(methodLiterals[0]);
    method->SetConstantPool(vm2->GetJSThread(), constPool2.GetTaggedValue());
    JSHandle<JSFunction> func = vm2->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm2->GetJSThread(), recordName);
    vm2->GetPGOProfiler()->ProfileCall(func.GetTaggedType());

    JSHandle<Method> method1 = vm_->GetFactory()->NewMethod(methodLiterals[0]);
    JSHandle<Method> method2 = vm_->GetFactory()->NewMethod(methodLiterals[1]);
    method1->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    method2->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSFunction> func1 = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method1);
    JSHandle<JSFunction> func2 = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method2);
    JSHandle<JSTaggedValue> recordName1(vm_->GetFactory()->NewFromStdString("test"));
    func1->SetModule(vm_->GetJSThread(), recordName);
    func2->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetPGOProfiler()->ProfileCall(func1.GetTaggedType());
    vm_->GetPGOProfiler()->ProfileCall(func2.GetTaggedType());

    JSNApi::DestroyJSVM(vm2);
    JSNApi::DestroyJSVM(vm_);

    PGOProfilerDecoder loader("ark-profiler5/profiler", 2);
    mkdir("ark-profiler5/profiler", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
    CString expectRecordName = "test";
    ASSERT_TRUE(loader.Match(expectRecordName, methodLiterals[1]->GetMethodId()));

    PGOProfilerDecoder loader1("ark-profiler5/modules.ap", 2);
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader1.LoadAndVerify(checksum));
    ASSERT_TRUE(!loader1.Match(expectRecordName, methodLiterals[1]->GetMethodId()));
#else
    ASSERT_TRUE(!loader1.LoadAndVerify(checksum));
    ASSERT_TRUE(loader1.Match(expectRecordName, methodLiterals[1]->GetMethodId()));
#endif

    unlink("ark-profiler5/modules.ap");
    rmdir("ark-profiler5/profiler");
    rmdir("ark-profiler5/");
}

HWTEST_F_L0(PGOProfilerTest, PGOProfilerDecoderNoHotMethod)
{
    const char *source = R"(
        .language ECMAScript
        .function void foo1(any a0, any a1, any a2) {}
    )";
    std::vector<MethodLiteral *> methodLiterals {};
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, "ark-profiler8.abc", methodLiterals);
    EXPECT_EQ(methodLiterals.size(), 1);  // number of methods
    mkdir("ark-profiler8/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler8/");
    vm_ = JSNApi::CreateJSVM(option);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf.get());
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);

    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiterals[0]);

    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetPGOProfiler()->ProfileCall(func.GetTaggedType());
    JSNApi::DestroyJSVM(vm_);

    PGOProfilerDecoder loader("ark-profiler8/modules.ap", 2);
    CString expectRecordName = "test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify(checksum));
    ASSERT_TRUE(!loader.Match(expectRecordName, methodLiterals[0]->GetMethodId()));
#else
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
    ASSERT_TRUE(loader.Match(expectRecordName, methodLiterals[0]->GetMethodId()));
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
    std::shared_ptr<JSPandaFile> pf =
        CreateJSPandaFile(sourceStream.str().c_str(), "ark-profiler9.abc", methodLiterals);
    EXPECT_EQ(methodLiterals.size(), 100);  // number of methods
    mkdir("ark-profiler9/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler9/");
    vm_ = JSNApi::CreateJSVM(option);
    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf.get());
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);

    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    for (int i = 61; i < 91; i++) {
        JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiterals[i]);
        method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
        JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
        func->SetModule(vm_->GetJSThread(), recordName);
        vm_->GetPGOProfiler()->ProfileCall(func.GetTaggedType());
        if (i % 3 == 0) {
            vm_->GetPGOProfiler()->ProfileCall(func.GetTaggedType());
        }
    }

    JSNApi::DestroyJSVM(vm_);

    PGOProfilerDecoder loader("ark-profiler9/modules.ap", 2);
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(loader.LoadAndVerify(checksum));
#else
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
#endif
    CString expectRecordName = "test";
    for (int i = 0; i < 100; i++) {
        EntityId methodId = methodLiterals[i]->GetMethodId();
        loader.MatchAndMarkMethod(expectRecordName, methodLiterals[i]->GetMethodName(pf.get(), methodId), methodId);
    }
    for (int i = 61; i < 91; i++) {
        if (i % 3 == 0) {
            ASSERT_TRUE(loader.Match(expectRecordName, methodLiterals[i]->GetMethodId()));
        } else {
#if defined(SUPPORT_ENABLE_ASM_INTERP)
            ASSERT_TRUE(!loader.Match(expectRecordName, methodLiterals[i]->GetMethodId()));
#else
            ASSERT_TRUE(loader.Match(expectRecordName, methodLiterals[i]->GetMethodId()));
#endif
        }
    }

    unlink("ark-profiler9/modules.ap");
    rmdir("ark-profiler9/");
}

HWTEST_F_L0(PGOProfilerTest, BinaryToText)
{
    mkdir("ark-profiler7/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    std::ofstream file("ark-profiler7/modules.ap");

    PGOProfilerHeader *header = nullptr;
    PGOProfilerHeader::Build(&header, PGOProfilerHeader::LastSize());
    std::unique_ptr<PGOPandaFileInfos> pandaFileInfos = std::make_unique<PGOPandaFileInfos>();
    std::unique_ptr<PGORecordDetailInfos> recordInfos = std::make_unique<PGORecordDetailInfos>(2);

    RuntimeOption option;
    vm_ = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
    pandaFileInfos->Sample(0x34556738);
    std::shared_ptr<MethodLiteral> methodLiteral = std::make_shared<MethodLiteral>(EntityId(61));
    auto *jsMethod =
        Method::Cast(vm_->GetFactory()->NewMethod(methodLiteral.get(), MemSpaceType::NON_MOVABLE).GetTaggedValue());

    ASSERT_TRUE(recordInfos->AddMethod("test", jsMethod, SampleMode::CALL_MODE, 1));
    ASSERT_FALSE(recordInfos->AddMethod("test", jsMethod, SampleMode::CALL_MODE, 1));
    ASSERT_FALSE(recordInfos->AddMethod("test", jsMethod, SampleMode::CALL_MODE, 1));

    pandaFileInfos->ProcessToBinary(file, header->GetPandaInfoSection());
    recordInfos->ProcessToBinary(nullptr, file, header);
    header->ProcessToBinary(file);
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

    ASSERT_TRUE(PGOProfilerManager::GetInstance()->TextToBinary("ark-profiler10/modules.text", "ark-profiler10/", 2));

    PGOProfilerDecoder loader("ark-profiler10/modules.ap", 2);
    ASSERT_TRUE(loader.LoadAndVerify(413775942));

    unlink("ark-profiler10/modules.ap");
    unlink("ark-profiler10/modules.text");
    rmdir("ark-profiler10");
}

HWTEST_F_L0(PGOProfilerTest, TextRecover)
{
    mkdir("ark-profiler11/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    std::ofstream file("ark-profiler11/modules.text");
    std::string result = "Profiler Version: 0.0.0.2\n";
    file.write(result.c_str(), result.size());
    result = "\nPanda file sumcheck list: [ 413775942 ]\n";
    file.write(result.c_str(), result.size());
    result = "\n_GLOBAL::funct_main_0: [ 1232/3/CALL_MODE/hello/ ]\n";
    file.write(result.c_str(), result.size());
    result = "\nrecordName: [ 234/100/HOTNESS_MODE/h#ello1/ ]\n";
    file.write(result.c_str(), result.size());
    file.close();

    ASSERT_TRUE(PGOProfilerManager::GetInstance()->TextToBinary("ark-profiler11/modules.text", "ark-profiler11/", 2));

    ASSERT_TRUE(PGOProfilerManager::GetInstance()->BinaryToText(
        "ark-profiler11/modules.ap", "ark-profiler11/modules_recover.text", 2));

    std::ifstream fileOrigin("ark-profiler11/modules.text");
    std::ifstream fileRecover("ark-profiler11/modules_recover.text");

    std::string lineOrigin;
    std::string lineRecover;
    // check content from origin and recovered profile line by line.
    while (std::getline(fileOrigin, lineOrigin)) {
        std::getline(fileRecover, lineRecover);
        ASSERT_EQ(lineOrigin, lineRecover);
    }

    fileOrigin.close();
    fileRecover.close();
    unlink("ark-profiler11/modules.ap");
    unlink("ark-profiler11/modules.text");
    unlink("ark-profiler11/modules_recover.text");
    rmdir("ark-profiler11");
}

HWTEST_F_L0(PGOProfilerTest, FailResetProfilerInWorker)
{
    const char *source = R"(
        .language ECMAScript
        .function void foo1(any a0, any a1, any a2) {}
    )";
    std::vector<MethodLiteral *> methodLiterals {};
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, "ark-profiler12.abc", methodLiterals);
    EXPECT_EQ(methodLiterals.size(), 1);  // number of methods
    mkdir("ark-profiler12/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    // Although enableProfle is set in option, but it will not work when isWorker is set.
    option.SetEnableProfile(true);
    option.SetIsWorker();
    option.SetProfileDir("ark-profiler12/");
    // PgoProfiler is disabled as default.
    vm_ = JSNApi::CreateJSVM(option);
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiterals[0]);

    JSHandle<ConstantPool> constPool = vm_->GetFactory()->NewConstantPool(4);
    constPool->SetJSPandaFile(pf.get());
    method->SetConstantPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetPGOProfiler()->ProfileCall(func.GetTaggedType());
    JSNApi::DestroyJSVM(vm_);

    // Loader
    PGOProfilerDecoder loader("ark-profiler12/modules.ap", 2);
    // path is empty()
    ASSERT_TRUE(!loader.LoadAndVerify(checksum));
    CString expectRecordName = "test";
    ASSERT_TRUE(loader.Match(expectRecordName, methodLiterals[0]->GetMethodId()));
    rmdir("ark-profiler12/");
}
}  // namespace panda::test
