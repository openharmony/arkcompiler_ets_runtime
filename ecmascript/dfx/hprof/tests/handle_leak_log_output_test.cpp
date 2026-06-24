/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_handle_scope.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
// Fixture enables ENABLE_HANDLE_LEAK_LOG_OUTPUT so that handle allocation
// outside any scope takes the if (EnableHandleLeakLogOutput()) -> HandleLeakDetect
// branch in ecma_handle_scope.cpp. Record content is gated by
// ENABLE_HITRACE_LOCAL_HANDLE_DETECT + ENABLE_BACKTRACE_LOCAL and, due to the
// "data/storage" backtrace filter, stays empty in unit tests; only the option
// accessor and the record API surface (Clear/Get) are covered.
class HandleLeakLogOutputTest : public testing::Test {
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
        JSRuntimeOptions options;
        options.SetArkProperties(options.GetArkProperties() | ArkProperties::ENABLE_HANDLE_LEAK_LOG_OUTPUT);
        ecmaVm_ = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(ecmaVm_ != nullptr) << "Cannot create EcmaVM";
        thread_ = ecmaVm_->GetJSThread();
        thread_->ManagedCodeBegin();
        ecmaVm_->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        if (ecmaVm_ != nullptr) {
            ecmaVm_->GetJSThread()->ManagedCodeEnd();
            JSNApi::DestroyJSVM(ecmaVm_);
            ecmaVm_ = nullptr;
        }
    }

    EcmaVM *ecmaVm_ {nullptr};
    JSThread *thread_ {nullptr};
};

// A VM created without the option must report the feature disabled.
HWTEST_F_L0(HandleLeakLogOutputTest, OptionDisabledByDefault)
{
    JSRuntimeOptions options;
    EcmaVM *disabledVm = JSNApi::CreateEcmaVM(options);
    ASSERT_TRUE(disabledVm != nullptr) << "Cannot create EcmaVM";
    JSThread *disabledThread = disabledVm->GetJSThread();
    disabledThread->ManagedCodeBegin();
    EXPECT_FALSE(disabledVm->GetJSOptions().EnableHandleLeakLogOutput());
    disabledThread->ManagedCodeEnd();
    JSNApi::DestroyJSVM(disabledVm);
}

#if defined(ENABLE_HITRACE_LOCAL_HANDLE_DETECT) && defined(ENABLE_BACKTRACE_LOCAL)
// Below: device-only (OHOS arm64). Exercises the PR's new Clear/Get record API.
// Record content stays empty due to the "data/storage" backtrace filter, so these
// cover API existence / callability / safety, not record production logic.

// Records are empty on a fresh VM with the option enabled.
HWTEST_F_L0(HandleLeakLogOutputTest, GetHandleLeakRecordsEmptyByDefault)
{
    ASSERT_TRUE(ecmaVm_->GetHandleLeakRecords().empty());
}

// Clear on empty records is a safe no-op and leaves the container valid.
HWTEST_F_L0(HandleLeakLogOutputTest, ClearHandleLeakRecordsOnEmpty)
{
    ecmaVm_->ClearHandleLeakRecords();
    ASSERT_TRUE(ecmaVm_->GetHandleLeakRecords().empty());
}

// With the option enabled, allocating a handle outside any scope invokes
// HandleLeakDetect. The backtrace filter keeps records empty in the test
// environment; verify Clear still leaves the container valid afterwards.
HWTEST_F_L0(HandleLeakLogOutputTest, ClearAfterOutOfScopeHandleAlloc)
{
    ASSERT_EQ(ecmaVm_->GetOpenHandleScopes(), 0U);
    ObjectFactory *factory = ecmaVm_->GetFactory();
    factory->NewTaggedArray(10);
    ecmaVm_->ClearHandleLeakRecords();
    ASSERT_TRUE(ecmaVm_->GetHandleLeakRecords().empty());
}
#endif // ENABLE_HITRACE_LOCAL_HANDLE_DETECT && ENABLE_BACKTRACE_LOCAL
}  // namespace panda::test
