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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/tests/test_helper.h"

#include <csetjmp>
#include <csignal>
using namespace panda::ecmascript;

namespace panda::test {
class EcmaVMTest : public BaseTestWithOutScope {
};

static sigjmp_buf g_env;
static bool g_abortFlag = false;
class EcmaVmIterator : public RootVisitor {
public:
    EcmaVmIterator() = default;
    ~EcmaVmIterator() = default;

    void VisitRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot slot) override
    {
    }

    void VisitRangeRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot start,
        [[maybe_unused]] ObjectSlot end) override
    {
        CHECK_NO_HANDLE_ALLOC;
    }

    void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
        [[maybe_unused]] ObjectSlot derived, [[maybe_unused]] uintptr_t baseOldObject) override
    {
    }

    static void ProcessHandleAlocAbort(int sig)
    {
        g_abortFlag = true;
        siglongjmp(g_env, sig);
    }

    static int RegisterSignal()
    {
        struct sigaction act;
        act.sa_handler = ProcessHandleAlocAbort;
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGQUIT);
        act.sa_flags = SA_RESETHAND;
        return sigaction(SIGABRT, &act, nullptr);
    }

    static void ResetAbortFlag()
    {
        g_abortFlag = false;
    }
};

/*
 * @tc.name: CreateEcmaVMInTwoWays
 * @tc.desc: Create EcmaVM in 2 ways, check the Options state
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaVMTest, CreateEcmaVMInTwoWays)
{
    RuntimeOption options;
    options.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM::SetMultiThreadCheck(true);
    EcmaVM *ecmaVm1 = JSNApi::CreateJSVM(options);
    EXPECT_TRUE(ecmaVm1->GetMultiThreadCheck());
    auto jsthread1 = ecmaVm1->GetJSThread();
    EXPECT_TRUE(jsthread1 != nullptr);

    std::thread t1([&]() {
        JSRuntimeOptions options2;
        options2.SetEnableArkTools(false);
        options2.SetEnableForceGC(false);
        options2.SetForceFullGC(false);
        options2.SetArkProperties(ArkProperties::GC_STATS_PRINT);
        options2.SetMemConfigProperty("jsHeap500");
        // A non-production gc strategy. Prohibit stw-gc 10 times.
        EcmaVM::SetMultiThreadCheck(false);
        EcmaVM *ecmaVm2 = JSNApi::CreateEcmaVM(options2);
        auto jsthread2 = ecmaVm2->GetJSThread();
        EXPECT_FALSE(ecmaVm2->GetMultiThreadCheck());
        EXPECT_TRUE(jsthread2 != nullptr);
        EXPECT_TRUE(ecmaVm1 != ecmaVm2);

        JSRuntimeOptions options1Out = ecmaVm1->GetJSOptions();
        JSRuntimeOptions options2Out = ecmaVm2->GetJSOptions();

        EXPECT_TRUE(&options1Out != &options2Out);

        EXPECT_TRUE(options1Out.EnableArkTools() != options2Out.EnableArkTools());
        EXPECT_TRUE(options1Out.EnableForceGC() != options2Out.EnableForceGC());
        EXPECT_TRUE(options1Out.ForceFullGC() != options2Out.ForceFullGC());
        EXPECT_TRUE(options1Out.GetArkProperties() != options2Out.GetArkProperties());
        EXPECT_TRUE(options2Out.GetHeapSize() == 500_MB);

        options2.SetAsmOpcodeDisableRange("1,10");
        options2.ParseAsmInterOption();
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleStart == 1); // 1 targer start
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleEnd == 10); // 10 targer end

        options2.SetAsmOpcodeDisableRange("0x1,0xa");
        options2.ParseAsmInterOption();
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleStart == 1); // 1 targer start
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleEnd == 10); // 10 targer end

        options2.SetAsmOpcodeDisableRange(",");
        options2.ParseAsmInterOption();
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleStart == 0);
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleEnd == kungfu::BYTECODE_STUB_END_ID);

        options2.SetAsmOpcodeDisableRange("@,@");
        options2.ParseAsmInterOption();
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleStart == 0);
        EXPECT_TRUE(options2.GetAsmInterParsedOption().handleEnd == kungfu::BYTECODE_STUB_END_ID);

        JSNApi::DestroyJSVM(ecmaVm2);
    });
    t1.join();
    JSNApi::DestroyJSVM(ecmaVm1);
}

HWTEST_F_L0(EcmaVMTest, DumpExceptionObject)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *ecmaVm = JSNApi::CreateJSVM(option);
    auto thread = ecmaVm->GetJSThread();
    int arkProperties = thread->GetEcmaVM()->GetJSOptions().GetArkProperties();
    ecmaVm->GetJSOptions().SetArkProperties(arkProperties | ArkProperties::EXCEPTION_BACKTRACE);
    EXPECT_TRUE(ecmaVm->GetJSOptions().EnableExceptionBacktrace());
    JSNApi::DestroyJSVM(ecmaVm);
}

HWTEST_F_L0(EcmaVMTest, TestHandleAlocate)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *ecmaVm = JSNApi::CreateJSVM(option);
    [[maybe_unused]]HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(ecmaVm);

    ObjectFactory *factory = ecmaVm->GetFactory();
    // JS_ARRAY_BUFFER
    factory->NewJSArrayBuffer(10);
    // JS_SHARED_ARRAY_BUFFER
    factory->NewJSSharedArrayBuffer(10);
    // PROMISE_REACTIONS
    factory->NewPromiseReaction();
    // PROMISE_CAPABILITY
    factory->NewPromiseCapability();
    // PROMISE_RECORD
    factory->NewPromiseRecord();
    // RESOLVING_FUNCTIONS_RECORD
    factory->NewResolvingFunctionsRecord();

    ASSERT_TRUE(EcmaVmIterator::RegisterSignal() != -1);
    EcmaVmIterator ecmaVmIterator;
    auto ret = sigsetjmp(g_env, 1);
    if (ret != SIGABRT) {
        ecmaVm->Iterate(ecmaVmIterator, VMRootVisitType::HEAP_SNAPSHOT);
    } else {
        EXPECT_TRUE(g_abortFlag);
    }

    ASSERT_TRUE(EcmaVmIterator::RegisterSignal() != -1);
    EcmaVmIterator::ResetAbortFlag();
    ret = sigsetjmp(g_env, 1);
    if (ret != SIGABRT) {
        ecmaVm->IterateHandle(ecmaVmIterator);
    } else {
        EXPECT_TRUE(g_abortFlag);
    }

    JSNApi::DestroyJSVM(ecmaVm);
}
}  // namespace panda::ecmascript
