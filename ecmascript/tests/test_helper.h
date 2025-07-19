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

#ifndef ECMASCRIPT_TESTS_TEST_HELPER_H
#define ECMASCRIPT_TESTS_TEST_HELPER_H

#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "gtest/gtest.h"

namespace panda::test {
using panda::ecmascript::EcmaHandleScope;
using panda::ecmascript::EcmaRuntimeCallInfo;
using panda::ecmascript::EcmaVM;
using panda::ecmascript::InterpretedFrame;
using panda::ecmascript::InterpretedBuiltinFrame;
using panda::ecmascript::InterpretedEntryFrame;
using panda::ecmascript::JSTaggedType;
using panda::ecmascript::JSTaggedValue;
using panda::ecmascript::JSThread;
using panda::ecmascript::NUM_MANDATORY_JSFUNC_ARGS;
using panda::ecmascript::JSRuntimeOptions;
using panda::ecmascript::JSFunction;
using panda::ecmascript::JSHandle;

#define HWTEST_F_L0(testsuit, testcase) HWTEST_F(testsuit, testcase, testing::ext::TestSize.Level0)
#define HWTEST_P_L0(testsuit, testcase) HWTEST_P(testsuit, testcase, testing::ext::TestSize.Level0)
#define EXPECT_EXCEPTION()                           \
    EXPECT_TRUE(thread->HasPendingException());      \
    EXPECT_TRUE(thread->GetException().IsJSError()); \
    thread->ClearException()

class TestHelper {
public:
    static EcmaRuntimeCallInfo* CreateEcmaRuntimeCallInfo(JSThread *thread, JSTaggedValue newTgt, uint32_t argvLength)
    {
        const uint8_t testDecodedSize = 2;
        // argvLength includes number of int64_t to store value and tag of function, 'this' and call args
        // It doesn't include new.target argument
        int32_t numActualArgs = argvLength / testDecodedSize + 1;
        JSTaggedType *sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());

        size_t frameSize = 0;
        if (thread->IsAsmInterpreter()) {
            frameSize = InterpretedEntryFrame::NumOfMembers() + numActualArgs;
        } else {
            frameSize = InterpretedFrame::NumOfMembers() + numActualArgs;
        }
        JSTaggedType *newSp = sp - frameSize;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for (int i = numActualArgs; i > 0; i--) {
            newSp[i - 1] = JSTaggedValue::Undefined().GetRawData();
        }
        EcmaRuntimeCallInfo *ecmaRuntimeCallInfo = reinterpret_cast<EcmaRuntimeCallInfo *>(newSp - 2);
        *(--newSp) = numActualArgs;
        *(--newSp) = reinterpret_cast<uintptr_t>(thread);
        ecmaRuntimeCallInfo->SetNewTarget(newTgt);
        return ecmaRuntimeCallInfo;
    }

    static JSTaggedType *SetupFrame(JSThread *thread, EcmaRuntimeCallInfo *info)
    {
        JSTaggedType *sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
        size_t frameSize = 0;
        if (thread->IsAsmInterpreter()) {
            // 2 means thread and numArgs
            frameSize = InterpretedEntryFrame::NumOfMembers() + info->GetArgsNumber() + NUM_MANDATORY_JSFUNC_ARGS + 2;
        } else {
            // 2 means thread and numArgs
            frameSize = InterpretedFrame::NumOfMembers() + info->GetArgsNumber() + NUM_MANDATORY_JSFUNC_ARGS + 2;
        }
        JSTaggedType *newSp = sp - frameSize;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

        InterpretedEntryFrame *state = reinterpret_cast<InterpretedEntryFrame *>(newSp) - 1;
        state->base.type = ecmascript::FrameType::INTERPRETER_ENTRY_FRAME;
        state->base.prev = sp;
        state->pc = nullptr;
        thread->SetCurrentSPFrame(newSp);
        return sp;
    }

    static void TearDownFrame(JSThread *thread, JSTaggedType *prev)
    {
        thread->SetCurrentSPFrame(prev);
    }

    // If you want to call once create, you can refer to BuiltinsMathTest for detail.
    static void CreateEcmaVMWithScope(EcmaVM *&instance, JSThread *&thread, EcmaHandleScope *&scope,
        bool tryLoadStubFile = false, bool useCInterpreter = false, bool startManagedCode = true,
        bool enableFastJit = false)
    {
        JSRuntimeOptions options;
        options.SetEnableForceGC(true);
        if (tryLoadStubFile) {
            options.SetEnableAsmInterpreter(true);
        }
        if (useCInterpreter) {
            options.SetEnableAsmInterpreter(false);
        }
        if (enableFastJit) {
            options.SetEnableJIT(true);
        }
        instance = JSNApi::CreateEcmaVM(options);
        instance->SetEnableForceGC(true);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        if (startManagedCode) {
            thread->ManagedCodeBegin();
        }
        scope = new EcmaHandleScope(thread);
    }

    static inline void DestroyEcmaVMWithScope(EcmaVM *instance, EcmaHandleScope *scope, bool exitManagedCode = true)
    {
        delete scope;
        scope = nullptr;
        if (exitManagedCode) {
            instance->GetJSThread()->ManagedCodeEnd();
        }
        instance->SetEnableForceGC(false);
        JSNApi::DestroyJSVM(instance);
    }

    static EcmaRuntimeCallInfo* CreateEcmaRuntimeCallInfo(JSThread *thread, std::vector<JSTaggedValue>& args,
        int32_t maxArgLen, JSTaggedValue thisValue = JSTaggedValue::Undefined())
    {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), maxArgLen);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(thisValue);
        for (size_t i = 0; i < args.size(); i++) {
            ecmaRuntimeCallInfo->SetCallArg(i, args[i]);
        }
        return ecmaRuntimeCallInfo;
    }

    static EcmaRuntimeCallInfo* CreateEcmaRuntimeCallInfo(JSThread *thread, JSHandle<JSFunction>& newTarget,
        std::vector<JSTaggedValue>& args, int32_t maxArgLen, JSTaggedValue thisValue = JSTaggedValue::Undefined())
    {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), maxArgLen);
        ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
        ecmaRuntimeCallInfo->SetThis(thisValue);
        for (size_t i = 0; i < args.size(); i++) {
            ecmaRuntimeCallInfo->SetCallArg(i, args[i]);
        }
        return ecmaRuntimeCallInfo;
    }
};

class BaseTestWithOutScope : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override {}

    void TearDown() override {}
};

template<bool icuPath = false>
class BaseTestWithScope : public BaseTestWithOutScope {
public:
    void SetUp() override
    {
        if (!icuPath) {
            TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        } else {
            JSRuntimeOptions options;
#if defined(PANDA_TARGET_LINUX) && defined(ICU_PATH)
        // for consistency requirement, use ohos_icu4j/data as icu-data-path
            options.SetIcuDataPath(ICU_PATH);
#endif
            options.SetEnableForceGC(true);
            instance = JSNApi::CreateEcmaVM(options);
            instance->SetEnableForceGC(true);
            ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
            thread = instance->GetJSThread();
            thread->ManagedCodeBegin();
            scope = new EcmaHandleScope(thread);
        }
        JSNApi::InitHybridVMEnv(thread->GetEcmaVM());
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};
}  // namespace panda::test
#endif  // ECMASCRIPT_TESTS_TEST_HELPER_H
