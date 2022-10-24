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
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    MethodLiteral *methodLiteral = new MethodLiteral(nullptr, EntityId(10));
    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiteral);
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetPGOProfiler()->Sample(func.GetTaggedType());
    JSNApi::DestroyJSVM(vm_);
    // Loader
    PGOProfilerLoader loader;
    loader.LoadProfiler("ark-profiler/profiler.aprof", 2);
    CString expectRecordName = "test";
    ASSERT_TRUE(!loader.Match(expectRecordName, EntityId(10)));
    unlink("ark-profiler/profile.aprof");
    rmdir("ark-profiler/");
}

HWTEST_F_L0(PGOProfilerTest, Sample1)
{
    mkdir("ark-profiler1/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    RuntimeOption option;
    option.SetEnableProfile(true);
    option.SetProfileDir("ark-profiler1/");
    vm_ = JSNApi::CreateJSVM(option);
    ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";

    MethodLiteral *methodLiteral = new MethodLiteral(nullptr, EntityId(10));
    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiteral);
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    for (int i = 0; i < 5; i++) {
        vm_->GetPGOProfiler()->Sample(func.GetTaggedType());
    }
    JSNApi::DestroyJSVM(vm_);

    // Loader
    PGOProfilerLoader loader;
    loader.LoadProfiler("ark-profiler1/profiler.aprof", 2);
    CString expectRecordName = "test";
    ASSERT_TRUE(loader.Match(expectRecordName, EntityId(10)));
    unlink("ark-profiler1/profile.aprof");
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

    MethodLiteral *methodLiteral = new MethodLiteral(nullptr, EntityId(10));
    MethodLiteral *methodLiteral1 = new MethodLiteral(nullptr, EntityId(15));
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
    loader.LoadProfiler("ark-profiler2/profiler.aprof", 2);
    CString expectRecordName = "test";
    CString expectRecordName1 = "test1";
    ASSERT_TRUE(!loader.Match(expectRecordName, EntityId(10)));
    ASSERT_TRUE(loader.Match(expectRecordName1, EntityId(15)));
    unlink("ark-profiler2/profile.aprof");
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

    MethodLiteral *methodLiteral = new MethodLiteral(nullptr, EntityId(10));
    JSHandle<Method> method = vm_->GetFactory()->NewMethod(methodLiteral);
    JSHandle<JSFunction> func = vm_->GetFactory()->NewJSFunction(vm_->GetGlobalEnv(), method);
    JSHandle<JSTaggedValue> recordName(vm_->GetFactory()->NewFromStdString("test"));
    func->SetModule(vm_->GetJSThread(), recordName);
    vm_->GetPGOProfiler()->Sample(func.GetTaggedType());
    JSNApi::DestroyJSVM(vm_);

    // Loader
    PGOProfilerLoader loader;
    loader.LoadProfiler("ark-profiler3/profiler.aprof", 2);
    CString expectRecordName = "test";
    ASSERT_TRUE(loader.Match(expectRecordName, EntityId(10)));
    rmdir("ark-profiler3/");
}
}  // namespace panda::test
