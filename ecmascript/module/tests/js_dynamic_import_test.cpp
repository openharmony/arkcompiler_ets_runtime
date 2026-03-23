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

#include "ecmascript/builtins/builtins_promise_job.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/module/js_dynamic_import.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class DynamicImportTest : public testing::Test {
public:
    static JSTaggedValue TestFunc(EcmaRuntimeCallInfo *argv)
    {
        return JSTaggedValue::Undefined();
    }

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
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

/**
 * @tc.name: DynamicImport_ExecuteNativeOrJsonModule_NativeModule
 * @tc.desc: Test ExecuteNativeOrJsonModule when module is not loaded (covers lines 46-80, else branch)
 * @tc.type: FUNC
 */
HWTEST_F_L0(DynamicImportTest, DynamicImport_ExecuteNativeOrJsonModule_NativeModule)
{
    ObjectFactory *factory = instance->GetFactory();
    ModuleManager *moduleManager = thread->GetModuleManager();

    CString specifier = "@ohos:test_module";

    // Verify module is not loaded before calling the function (ensures else branch)
    EXPECT_FALSE(moduleManager->IsLocalModuleLoaded(specifier));

    JSHandle<JSFunction> resolve = factory->NewJSFunction(thread->GetEcmaVM()->GetGlobalEnv(),
        reinterpret_cast<void *>(DynamicImportTest::TestFunc));

    JSHandle<JSFunction> reject = factory->NewJSFunction(thread->GetEcmaVM()->GetGlobalEnv(),
        reinterpret_cast<void *>(DynamicImportTest::TestFunc));

    JSHandle<JSPromiseReactionsFunction> resolveHandle(resolve);
    JSHandle<JSPromiseReactionsFunction> rejectHandle(reject);

    // Call ExecuteNativeOrJsonModule - should take the else branch (lines 46-80)
    JSTaggedValue result = DynamicImport::ExecuteNativeOrJsonModule(
        thread, specifier, ModuleTypes::NATIVE_MODULE, resolveHandle, rejectHandle, nullptr);

    // Verify the function executed successfully
    EXPECT_FALSE(thread->HasPendingException());
    EXPECT_TRUE(result.IsUndefined());

    // Verify module was loaded and added to ModuleManager
    EXPECT_TRUE(moduleManager->IsLocalModuleLoaded(specifier));

    // Verify module status is correct
    JSHandle<SourceTextModule> module = moduleManager->HostGetImportedModule(specifier);
    EXPECT_EQ(module->GetStatus(), ModuleStatus::INSTANTIATED);
}

}  // namespace panda::test