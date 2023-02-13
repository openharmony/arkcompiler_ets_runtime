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

#include "gtest/gtest.h"

#include "ecmascript/dfx/pgo_profiler/pgo_profiler_loader.h"
#include "ecmascript/dfx/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

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

protected:
    EcmaVM *vm_ = nullptr;
};

HWTEST_F_L0(PGOProfilerTest, Sample)
{
    mkdir("ark-profiler/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler/");
    vm_ = JSNApi::CreateJSVM(option);
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    MethodLiteral *methodLiteral = new MethodLiteral(EntityId(61));
    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiteral);
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetPGOProfiler()->Sample(func.GetTaggedType());
    JSNApi::DestroyJSVM(vm_);
    // Loader
    PGOProfilerLoader loader;
    ASSERT_TRUE(loader.LoadAndVerify("ark-profiler/modules.ap", 2, checksum));
    CString expectRecordName = "test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(!loader.Match(expectRecordName, EntityId(61)));
#else
    ASSERT_TRUE(loader.Match(expectRecordName, EntityId(61)));
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
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    MethodLiteral *methodLiteral = new MethodLiteral(EntityId(70));
    MethodLiteral *methodLiteral1 = new MethodLiteral(EntityId(75));
    MethodLiteral *methodLiteral2 = new MethodLiteral(EntityId(80));
    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiteral);
    JSHandle<Method> method1 = vm_->GetFactory()->NewMethod(methodLiteral1);
    JSHandle<Method> method2 = vm_->GetFactory()->NewMethod(methodLiteral2);
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSFunction> func1 = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method1);
    JSHandle<JSFunction> func2 = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method2);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    func1->SetModule(vm_->GetJSThread(), recordName);
    func2->SetModule(vm_->GetJSThread(), recordName);
    for (int i = 0; i < 5; i++) {
        vm_->GetPGOProfiler()->Sample(func.GetTaggedType());
    }
    for (int i = 0; i < 50; i++) {
        vm_->GetPGOProfiler()->Sample(func2.GetTaggedType());
    }
    vm_->GetPGOProfiler()->Sample(func1.GetTaggedType());
    JSNApi::DestroyJSVM(vm_);

    // Loader
    PGOProfilerLoader loader;
    ASSERT_TRUE(loader.LoadAndVerify("ark-profiler1/modules.ap", 2, checksum));
    CString expectRecordName = "test";
    ASSERT_TRUE(loader.Match(expectRecordName, EntityId(70)));
    ASSERT_TRUE(loader.Match(expectRecordName, EntityId(80)));
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(!loader.Match(expectRecordName, EntityId(75)));
#else
    ASSERT_TRUE(loader.Match(expectRecordName, EntityId(75)));
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
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);

    MethodLiteral *methodLiteral = new MethodLiteral(EntityId(61));
    MethodLiteral *methodLiteral1 = new MethodLiteral(EntityId(62));
    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiteral);
    JSHandle<Method> method1 = vm_->GetFactory()->NewMethod(methodLiteral1);
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    JSHandle<JSFunction> func1 = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method1);
    JSHandle<JSTaggedValue> recordName1(vm_->GetFactory()->NewFromStdString("test1"));
    func1->SetModule(vm_->GetJSThread(), recordName1);
    vm_->GetPGOProfiler()->Sample(func.GetTaggedType());
    for (int i = 0; i < 5; i++) {
        vm_->GetPGOProfiler()->Sample(func1.GetTaggedType());
    }
    JSNApi::DestroyJSVM(vm_);

    // Loader
    PGOProfilerLoader loader;
    ASSERT_TRUE(loader.LoadAndVerify("ark-profiler2/modules.ap", 2, checksum));
    CString expectRecordName = "test";
    CString expectRecordName1 = "test1";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(!loader.Match(expectRecordName, EntityId(61)));
#else
    ASSERT_TRUE(loader.Match(expectRecordName, EntityId(61)));
#endif
    ASSERT_TRUE(loader.Match(expectRecordName1, EntityId(62)));
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
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    MethodLiteral *methodLiteral = new MethodLiteral(EntityId(61));
    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiteral);
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetPGOProfiler()->Sample(func.GetTaggedType());
    JSNApi::DestroyJSVM(vm_);

    // Loader
    PGOProfilerLoader loader;
    // path is empty()
    ASSERT_TRUE(!loader.Load("ark-profiler3/modules.ap", 2));
    CString expectRecordName = "test";
    ASSERT_TRUE(loader.Match(expectRecordName, EntityId(61)));
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
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    JSHandle<JSArray> array = vm_->GetFactory()->NewJSArray();
    vm_->GetPGOProfiler()->Sample(array.GetTaggedType());

    // RecordName is hole
    MethodLiteral *methodLiteral = new MethodLiteral(EntityId(61));
    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiteral);
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    func->SetModule(vm_->GetJSThread(), JSTaggedValue::Hole());
    vm_->GetPGOProfiler()->Sample(func.GetTaggedType());
    JSNApi::DestroyJSVM(vm_);

    PGOProfilerLoader loader;
    // path is empty()
    ASSERT_TRUE(!loader.Load("", 2));
    // path size greater than PATH_MAX
    char path[PATH_MAX + 1] = {'0'};
    loader.Load(path, 4);
}

HWTEST_F_L0(PGOProfilerTest, PGOProfilerDoubleVM)
{
    mkdir("ark-profiler5/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    // outDir is empty
    option.SetProfileDir("ark-profiler5/");
    vm_ = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
    auto vm2 = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm2 != nullptr) << "Cannot create Runtime";

    MethodLiteral *methodLiteral = new MethodLiteral(EntityId(70));
    MethodLiteral *methodLiteral1 = new MethodLiteral(EntityId(75));
    JSHandle<Method> method = vm2->GetFactory()->NewMethod(methodLiteral);
    JSHandle<JSFunction> func = vm2->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm2->GetJSThread(), recordName);
    vm2->GetPGOProfiler()->Sample(func.GetTaggedType());

    JSHandle<Method> method1 = vm_->GetFactory()->NewMethod(methodLiteral);
    JSHandle<Method> method2 = vm_->GetFactory()->NewMethod(methodLiteral1);
    JSHandle<JSFunction> func1 = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method1);
    JSHandle<JSFunction> func2 = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method2);
    JSHandle<JSTaggedValue> recordName1(vm_->GetFactory()->NewFromStdString("test"));
    func1->SetModule(vm_->GetJSThread(), recordName);
    func2->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetPGOProfiler()->Sample(func1.GetTaggedType());
    vm_->GetPGOProfiler()->Sample(func2.GetTaggedType());

    JSNApi::DestroyJSVM(vm2);
    JSNApi::DestroyJSVM(vm_);

    PGOProfilerLoader loader;
    mkdir("ark-profiler5/profiler", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    ASSERT_TRUE(!loader.Load("ark-profiler5/profiler", 2));
    CString expectRecordName = "test";
    ASSERT_TRUE(loader.Match(expectRecordName, EntityId(75)));

    loader.Load("ark-profiler5/modules.ap", 2);
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(!loader.Match(expectRecordName, EntityId(75)));
#else
    ASSERT_TRUE(loader.Match(expectRecordName, EntityId(75)));
#endif

    unlink("ark-profiler5/modules.ap");
    rmdir("ark-profiler5/profiler");
    rmdir("ark-profiler5/");
}

HWTEST_F_L0(PGOProfilerTest, DoubleRecordNameFormat)
{
    mkdir("ark-profiler7/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    std::ofstream file("ark-profiler7/modules.ap");
    std::string result = "recordName:[123/223/hello]\n";
    file.write(result.c_str(), result.size());
    result = "recordName:[1232/3/hello]\n";
    file.write(result.c_str(), result.size());
    file.close();
    PGOProfilerLoader loader;
    loader.Load("ark-profiler7/modules.ap", 2);

    unlink("ark-profiler7/modules.ap");
    rmdir("ark-profiler7");
}

HWTEST_F_L0(PGOProfilerTest, PGOProfilerLoaderNoHotMethod)
{
    mkdir("ark-profiler8/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler8/");
    vm_ = JSNApi::CreateJSVM(option);
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);

    MethodLiteral *methodLiteral = new MethodLiteral(EntityId(61));
    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiteral);
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetPGOProfiler()->Sample(func.GetTaggedType());
    JSNApi::DestroyJSVM(vm_);

    PGOProfilerLoader loader;
    ASSERT_TRUE(loader.LoadAndVerify("ark-profiler8/modules.ap", 2, checksum));
    CString expectRecordName = "test";
#if defined(SUPPORT_ENABLE_ASM_INTERP)
    ASSERT_TRUE(!loader.Match(expectRecordName, EntityId(61)));
#else
    ASSERT_TRUE(loader.Match(expectRecordName, EntityId(61)));
#endif

    unlink("ark-profiler8/modules.ap");
    rmdir("ark-profiler8/");
}

HWTEST_F_L0(PGOProfilerTest, PGOProfilerPostTask)
{
    mkdir("ark-profiler9/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler9/");
    vm_ = JSNApi::CreateJSVM(option);
    uint32_t checksum = 304293;
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(checksum);

    for (int i = 0; i < 5; i++) {
        PGOProfilerManager::GetInstance()->PostSaveTask();
        PGOProfilerManager::GetInstance()->TerminateSaveTask();
    }

    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    for (int i = 61; i < 91; i++) {
        MethodLiteral *methodLiteral = new MethodLiteral(EntityId(i));
        JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiteral);
        JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
        func->SetModule(vm_->GetJSThread(), recordName);
        vm_->GetPGOProfiler()->Sample(func.GetTaggedType());
        if (i % 3 == 0) {
            vm_->GetPGOProfiler()->Sample(func.GetTaggedType());
        }
    }

    JSNApi::DestroyJSVM(vm_);

    PGOProfilerLoader loader;
    ASSERT_TRUE(loader.LoadAndVerify("ark-profiler9/modules.ap", 2, checksum));
    CString expectRecordName = "test";
    for (int i = 61; i < 91; i++) {
        if (i % 3 == 0) {
            ASSERT_TRUE(loader.Match(expectRecordName, EntityId(i)));
        } else {
#if defined(SUPPORT_ENABLE_ASM_INTERP)
            ASSERT_TRUE(!loader.Match(expectRecordName, EntityId(i)));
#else
            ASSERT_TRUE(loader.Match(expectRecordName, EntityId(i)));
#endif
        }
    }

    unlink("ark-profiler9/modules.ap");
    rmdir("ark-profiler9/");
}
}  // namespace panda::test
