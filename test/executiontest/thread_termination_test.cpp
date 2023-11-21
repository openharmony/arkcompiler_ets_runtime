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

#include <condition_variable>

#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::test {
using namespace panda;
using namespace panda::ecmascript;
using FunctionCallbackInfo = Local<JSValueRef>(*)(JsiRuntimeCallInfo *);
std::condition_variable g_semaphore;

class ThreadTerminationTest : public testing::Test {
public:
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
        TestHelper::CreateEcmaVMWithScope(vm_, thread_, scope_);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(vm_, scope_);
    }

    static Local<JSValueRef> TerminateThread(JsiRuntimeCallInfo *runtimeCallInfo);
    static Local<JSValueRef> Fail(JsiRuntimeCallInfo *runtimeCallInfo);
    static Local<JSValueRef> Signal(JsiRuntimeCallInfo *runtimeCallInfo);

    static void RegisterGlobalTemplate(const EcmaVM *vm, FunctionCallbackInfo terminate, FunctionCallbackInfo fail,
                                       FunctionCallbackInfo signal)
    {
        [[maybe_unused]] EcmaHandleScope handleScope(vm->GetJSThread());
        Local<ObjectRef> globalObj = JSNApi::GetGlobalObject(vm);
        globalObj->Set(vm, StringRef::NewFromUtf8(vm, "terminate"), FunctionRef::New(
            const_cast<panda::EcmaVM*>(vm), terminate));
        globalObj->Set(vm, StringRef::NewFromUtf8(vm, "fail"), FunctionRef::New(
            const_cast<panda::EcmaVM*>(vm), fail));
        globalObj->Set(vm, StringRef::NewFromUtf8(vm, "signal"), FunctionRef::New(
            const_cast<panda::EcmaVM*>(vm), Signal));
    }

protected:
    EcmaVM *vm_ {nullptr};
    JSThread *thread_ = {nullptr};
    EcmaHandleScope *scope_ {nullptr};
};

class TerminatorThread {
public:
    explicit TerminatorThread(EcmaVM *vm) : vm_(vm) {}
    ~TerminatorThread() = default;

    void Run()
    {
        thread_ = std::thread([this]() {
            std::unique_lock<std::mutex> lock(mutex_);
            g_semaphore.wait(lock);
            DFXJSNApi::TerminateExecution(vm_);
        });
    }

    void RunWithSleep()
    {
        thread_ = std::thread([this]() {
            usleep(1000000); // 1000000 : 1s for loop to execute
            DFXJSNApi::TerminateExecution(vm_);
        });
    }

    void Join()
    {
        thread_.join();
    }

private:
    EcmaVM *vm_ {nullptr};
    std::mutex mutex_;
    std::thread thread_;
};

Local<JSValueRef> ThreadTerminationTest::TerminateThread(JsiRuntimeCallInfo *runtimeCallInfo)
{
    EcmaVM *vm = runtimeCallInfo->GetVM();
    DFXJSNApi::TerminateExecution(vm);
    return JSValueRef::Undefined(vm);
}

Local<JSValueRef> ThreadTerminationTest::Fail([[maybe_unused]]JsiRuntimeCallInfo *runtimeCallInfo)
{
    UNREACHABLE();
}

Local<JSValueRef> ThreadTerminationTest::Signal(JsiRuntimeCallInfo *runtimeCallInfo)
{
    EcmaVM *vm = runtimeCallInfo->GetVM();
    g_semaphore.notify_one();
    return JSValueRef::Undefined(vm);
}

HWTEST_F_L0(ThreadTerminationTest, TerminateFromThreadItself)
{
    JSThread *thread = vm_->GetAssociatedJSThread();
    EcmaContext::MountContext(thread);
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    RegisterGlobalTemplate(vm_, TerminateThread, Fail, Signal);
    TryCatch tryCatch(vm_);
    std::string baseFileName = MODULE_ABC_PATH "termination_1.abc";
    JSNApi::Execute(vm_, baseFileName, "termination_1");
    EXPECT_TRUE(tryCatch.HasCaught());
    EcmaContext::UnmountContext(thread);
}

HWTEST_F_L0(ThreadTerminationTest, TerminateFromOtherThread)
{
    JSThread *thread = vm_->GetAssociatedJSThread();
    EcmaContext::MountContext(thread);
    TerminatorThread terminatorThread(vm_);
    terminatorThread.Run();
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    RegisterGlobalTemplate(vm_, TerminateThread, Fail, Signal);
    TryCatch tryCatch(vm_);
    std::string baseFileName = MODULE_ABC_PATH "termination_2.abc";
    JSNApi::Execute(vm_, baseFileName, "termination_2");
    EXPECT_TRUE(tryCatch.HasCaught());
    terminatorThread.Join();
    EcmaContext::UnmountContext(thread);
}

HWTEST_F_L0(ThreadTerminationTest, TerminateFromOtherThreadWithoutCall)
{
    JSThread *thread = vm_->GetAssociatedJSThread();
    EcmaContext::MountContext(thread);
    TerminatorThread terminatorThread(vm_);
    terminatorThread.RunWithSleep();
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    RegisterGlobalTemplate(vm_, TerminateThread, Fail, Signal);
    TryCatch tryCatch(vm_);
    std::string baseFileName = MODULE_ABC_PATH "termination_3.abc";
    JSNApi::Execute(vm_, baseFileName, "termination_3");
    EXPECT_TRUE(tryCatch.HasCaught());
    terminatorThread.Join();
    EcmaContext::UnmountContext(thread);
}

HWTEST_F_L0(ThreadTerminationTest, TerminateClearArrayJoinStack)
{
    JSThread *thread = vm_->GetAssociatedJSThread();
    EcmaContext::MountContext(thread);
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    RegisterGlobalTemplate(vm_, TerminateThread, Fail, Signal);
    TryCatch tryCatch(vm_);
    std::string baseFileName = MODULE_ABC_PATH "termination_4.abc";
    JSNApi::Execute(vm_, baseFileName, "termination_4");
    EXPECT_TRUE(tryCatch.HasCaught());
    EcmaContext::UnmountContext(thread);
}

HWTEST_F_L0(ThreadTerminationTest, TerminateInMicroTask)
{
    JSThread *thread = vm_->GetAssociatedJSThread();
    EcmaContext::MountContext(thread);
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    RegisterGlobalTemplate(vm_, TerminateThread, Fail, Signal);
    TryCatch tryCatch(vm_);
    std::string baseFileName = MODULE_ABC_PATH "termination_5.abc";
    JSNApi::Execute(vm_, baseFileName, "termination_5");
    EcmaContext::UnmountContext(thread);
}

HWTEST_F_L0(ThreadTerminationTest, TerminateWithoutExecutingMicroTask)
{
    JSThread *thread = vm_->GetAssociatedJSThread();
    EcmaContext::MountContext(thread);
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    RegisterGlobalTemplate(vm_, TerminateThread, Fail, Signal);
    TryCatch tryCatch(vm_);
    std::string baseFileName = MODULE_ABC_PATH "termination_6.abc";
    JSNApi::Execute(vm_, baseFileName, "termination_6");
    EXPECT_TRUE(tryCatch.HasCaught());
    EcmaContext::UnmountContext(thread);
}
} // namespace panda::test
