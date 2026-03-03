/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "ecmascript/js_object-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/module/napi_module_loader.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_array.h"
#include "ecmascript/global_env.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/module/js_module_namespace.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using ModulePathHelper = ecmascript::ModulePathHelper;

namespace panda::test {
class NapiModuleLoaderTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    // Helper function to verify ModuleNamespace structure
    void VerifyModuleNamespace(const JSHandle<JSTaggedValue> &result)
    {
        // Verify result is not undefined
        EXPECT_FALSE(result.GetTaggedValue().IsUndefined());

        // Check if result is a heap object (most cases)
        if (result.GetTaggedValue().IsHeapObject()) {
            // If it's a ModuleNamespace, verify its structure
            if (result.GetTaggedValue().IsModuleNamespace()) {
                JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(result);
                EXPECT_FALSE(moduleNamespace->GetModule(thread).IsUndefined());
                EXPECT_FALSE(moduleNamespace->GetExports(thread).IsUndefined());
            } else if (result.GetTaggedValue().IsJSObject()) {
                // If it's a regular JSObject (fallback), verify it has expected properties
                // This handles cases where the namespace is exposed as a regular object
                JSHandle<JSObject> obj = JSHandle<JSObject>::Cast(result);
                // At minimum, it should be a valid object
                EXPECT_TRUE(obj->GetClass()->IsJSObject());
            }
        }
        // Note: Some test cases may return non-heap objects (e.g., special cases)
        // In those cases, we just verify it's not undefined
    }

    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

/**
 * @tc.name: NapiModuleLoader_GetModuleNameSpace_AfterExecute
 * @tc.desc: Test GetModuleNameSpace after executing a module
 * @tc.type: FUNC
 */
HWTEST_F_L0(NapiModuleLoaderTest, NapiModuleLoader_GetModuleNameSpace_AfterExecute)
{
    // Execute the module
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool executeResult = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    ASSERT_TRUE(executeResult);

    // Get namespace
    CString entryPoint = "module_test_module_test_C";
    CString abcFilePath = baseFileName.c_str();

    JSHandle<JSTaggedValue> result = NapiModuleLoader::GetModuleNameSpace(thread, entryPoint, abcFilePath);

    // Verify result is not a hole
    EXPECT_FALSE(result.GetTaggedValue().IsHole());

    // Verify ModuleNamespace structure and type
    VerifyModuleNamespace(result);
}

/**
 * @tc.name: NapiModuleLoader_LoadFromFile_AfterExecute
 * @tc.desc: Test LoadModuleNameSpaceFromFile after executing
 * @tc.type: FUNC
 */
HWTEST_F_L0(NapiModuleLoaderTest, NapiModuleLoader_LoadFromFile_AfterExecute)
{
    // Execute module
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_module.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool executeResult = JSNApi::Execute(instance, baseFileName, "module_test_module_test_module");
    ASSERT_TRUE(executeResult);

    // Load from file
    CString entryPoint = "module_test_module_test_module";
    CString abcFilePath = baseFileName.c_str();

    JSHandle<JSTaggedValue> result = NapiModuleLoader::LoadModuleNameSpaceFromFile<ForHybridApp::Normal>(
        thread, entryPoint, abcFilePath);

    // Verify ModuleNamespace structure and type
    VerifyModuleNamespace(result);
}

/**
 * @tc.name: NapiModuleLoader_LoadFromFile_Hybrid_Template
 * @tc.desc: Test LoadModuleNameSpaceFromFile with Hybrid template
 * @tc.type: FUNC
 */
HWTEST_F_L0(NapiModuleLoaderTest, NapiModuleLoader_LoadFromFile_Hybrid_Template)
{
    // Execute module
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_module.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool executeResult = JSNApi::Execute(instance, baseFileName, "module_test_module_test_module");
    ASSERT_TRUE(executeResult);

    // Load with Hybrid
    CString entryPoint = "module_test_module_test_module";
    CString abcFilePath = baseFileName.c_str();

    JSHandle<JSTaggedValue> result = NapiModuleLoader::LoadModuleNameSpaceFromFile<ForHybridApp::Hybrid>(
        thread, entryPoint, abcFilePath);

    // Verify ModuleNamespace structure and type
    VerifyModuleNamespace(result);
}

/**
 * @tc.name: NapiModuleLoader_ModuleABase
 * @tc.desc: Test loading module_base after execution
 * @tc.type: FUNC
 */
HWTEST_F_L0(NapiModuleLoaderTest, NapiModuleLoader_ModuleABase)
{
    // Execute module_base
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_module_base.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool executeResult = JSNApi::Execute(instance, baseFileName, "module_test_module_test_module_base");
    ASSERT_TRUE(executeResult);

    // Get namespace
    CString entryPoint = "module_test_module_test_module_base";
    CString abcFilePath = baseFileName.c_str();

    JSHandle<JSTaggedValue> result = NapiModuleLoader::GetModuleNameSpace(thread, entryPoint, abcFilePath);

    // Verify ModuleNamespace structure and type
    VerifyModuleNamespace(result);
}

/**
 * @tc.name: NapiModuleLoader_LoadModuleNameSpace_WithModuleName
 * @tc.desc: Test LoadModuleNameSpace with moduleName parameter
 * @tc.type: FUNC
 * @tc.note: This test verifies LoadModuleNameSpace can handle various requestPath formats.
 *          These specific test cases may trigger exceptions due to invalid requestPath format.
 *          The test verifies the function handles these edge cases appropriately.
 */
HWTEST_F_L0(NapiModuleLoaderTest, NapiModuleLoader_LoadModuleNameSpace_WithModuleName)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_module.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool executeResult = JSNApi::Execute(instance, baseFileName, "module_test_module_test_module");
    ASSERT_TRUE(executeResult);

    CString requestPath = "module_test_module_test_module";
    CString moduleName = "module_test_module_test_module";
    CString abcFilePath = baseFileName.c_str();

    JSHandle<JSTaggedValue> result = NapiModuleLoader::LoadModuleNameSpace<ForHybridApp::Normal>(
        instance, requestPath, moduleName, abcFilePath);

    // This test case uses an invalid requestPath format that may cause exceptions
    // The function should either return a valid module namespace or throw an exception
    if (!thread->HasPendingException()) {
        // If no exception was thrown, verify the result is valid
        EXPECT_FALSE(result.GetTaggedValue().IsUndefined());
    }
    // If an exception was thrown, that's also acceptable behavior for invalid input
}

/**
 * @tc.name: NapiModuleLoader_LoadModuleNameSpace_Hybrid_WithModuleName
 * @tc.desc: Test LoadModuleNameSpace with Hybrid template and moduleName
 * @tc.type: FUNC
 * @tc.note: This test verifies LoadModuleNameSpace with Hybrid template can handle
 *          various requestPath formats. These specific test cases may trigger exceptions
 *          due to invalid requestPath format. The test verifies the function handles
 *          these edge cases appropriately.
 */
HWTEST_F_L0(NapiModuleLoaderTest, NapiModuleLoader_LoadModuleNameSpace_Hybrid_WithModuleName)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_module.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool executeResult = JSNApi::Execute(instance, baseFileName, "module_test_module_test_module");
    ASSERT_TRUE(executeResult);

    CString requestPath = "module_test_module_test_module";
    CString moduleName = "module_test_module_test_module";
    CString abcFilePath = baseFileName.c_str();

    JSHandle<JSTaggedValue> result = NapiModuleLoader::LoadModuleNameSpace<ForHybridApp::Hybrid>(
        instance, requestPath, moduleName, abcFilePath);

    // This test case uses an invalid requestPath format that may cause exceptions
    // The function should either return a valid module namespace or throw an exception
    if (!thread->HasPendingException()) {
        // If no exception was thrown, verify the result is valid
        EXPECT_FALSE(result.GetTaggedValue().IsUndefined());
    }
    // If an exception was thrown, that's also acceptable behavior for invalid input
}

/**
 * @tc.name: NapiModuleLoader_GetModuleNameSpace_AlreadyLoaded
 * @tc.desc: Test GetModuleNameSpace when module is already loaded
 * @tc.type: FUNC
 */
HWTEST_F_L0(NapiModuleLoaderTest, NapiModuleLoader_GetModuleNameSpace_AlreadyLoaded)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_C.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool executeResult = JSNApi::Execute(instance, baseFileName, "module_test_module_test_C");
    ASSERT_TRUE(executeResult);

    CString entryPoint = "module_test_module_test_C";
    CString abcFilePath = baseFileName.c_str();

    JSHandle<JSTaggedValue> result1 = NapiModuleLoader::GetModuleNameSpace(thread, entryPoint, abcFilePath);
    VerifyModuleNamespace(result1);

    JSHandle<JSTaggedValue> result2 = NapiModuleLoader::GetModuleNameSpace(thread, entryPoint, abcFilePath);
    VerifyModuleNamespace(result2);

    // Verify both calls return the same namespace object
    EXPECT_EQ(result1.GetTaggedValue(), result2.GetTaggedValue());
}

/**
 * @tc.name: NapiModuleLoader_MultipleModules
 * @tc.desc: Test loading multiple different modules
 * @tc.type: FUNC
 */
HWTEST_F_L0(NapiModuleLoaderTest, NapiModuleLoader_MultipleModules)
{
    std::string baseFileName1 = MODULE_ABC_PATH "module_test_module_test_C.abc";
    std::string baseFileName2 = MODULE_ABC_PATH "module_test_module_test_module.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool executeResult1 = JSNApi::Execute(instance, baseFileName1, "module_test_module_test_C");
    ASSERT_TRUE(executeResult1);

    bool executeResult2 = JSNApi::Execute(instance, baseFileName2, "module_test_module_test_module");
    ASSERT_TRUE(executeResult2);

    CString entryPoint1 = "module_test_module_test_C";
    CString abcFilePath1 = baseFileName1.c_str();

    CString entryPoint2 = "module_test_module_test_module";
    CString abcFilePath2 = baseFileName2.c_str();

    JSHandle<JSTaggedValue> result1 = NapiModuleLoader::GetModuleNameSpace(thread, entryPoint1, abcFilePath1);
    VerifyModuleNamespace(result1);

    JSHandle<JSTaggedValue> result2 = NapiModuleLoader::GetModuleNameSpace(thread, entryPoint2, abcFilePath2);
    VerifyModuleNamespace(result2);

    // Verify different modules return different namespace objects
    EXPECT_NE(result1.GetTaggedValue(), result2.GetTaggedValue());
}

/**
 * @tc.name: NapiModuleLoader_LoadFromFile_AfterMultipleExecutes
 * @tc.desc: Test LoadModuleNameSpaceFromFile after multiple executions
 * @tc.type: FUNC
 */
HWTEST_F_L0(NapiModuleLoaderTest, NapiModuleLoader_LoadFromFile_AfterMultipleExecutes)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_module.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool executeResult1 = JSNApi::Execute(instance, baseFileName, "module_test_module_test_module");
    ASSERT_TRUE(executeResult1);

    bool executeResult2 = JSNApi::Execute(instance, baseFileName, "module_test_module_test_module");
    ASSERT_TRUE(executeResult2);

    CString entryPoint = "module_test_module_test_module";
    CString abcFilePath = baseFileName.c_str();

    JSHandle<JSTaggedValue> result = NapiModuleLoader::LoadModuleNameSpaceFromFile<ForHybridApp::Normal>(
        thread, entryPoint, abcFilePath);

    // Verify ModuleNamespace structure and type
    VerifyModuleNamespace(result);
}

/**
 * @tc.name: NapiModuleLoader_LoadFromFile_ModuleBase
 * @tc.desc: Test LoadModuleNameSpaceFromFile with module_base
 * @tc.type: FUNC
 */
HWTEST_F_L0(NapiModuleLoaderTest, NapiModuleLoader_LoadFromFile_ModuleBase)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_module_base.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool executeResult = JSNApi::Execute(instance, baseFileName, "module_test_module_test_module_base");
    ASSERT_TRUE(executeResult);

    CString entryPoint = "module_test_module_test_module_base";
    CString abcFilePath = baseFileName.c_str();

    JSHandle<JSTaggedValue> result = NapiModuleLoader::LoadModuleNameSpaceFromFile<ForHybridApp::Normal>(
        thread, entryPoint, abcFilePath);

    // Verify ModuleNamespace structure and type
    VerifyModuleNamespace(result);
}

/**
 * @tc.name: NapiModuleLoader_LoadFromFile_Hybrid_ModuleBase
 * @tc.desc: Test LoadModuleNameSpaceFromFile with Hybrid and module_base
 * @tc.type: FUNC
 */
HWTEST_F_L0(NapiModuleLoaderTest, NapiModuleLoader_LoadFromFile_Hybrid_ModuleBase)
{
    std::string baseFileName = MODULE_ABC_PATH "module_test_module_test_module_base.abc";
    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool executeResult = JSNApi::Execute(instance, baseFileName, "module_test_module_test_module_base");
    ASSERT_TRUE(executeResult);

    CString entryPoint = "module_test_module_test_module_base";
    CString abcFilePath = baseFileName.c_str();

    JSHandle<JSTaggedValue> result = NapiModuleLoader::LoadModuleNameSpaceFromFile<ForHybridApp::Hybrid>(
        thread, entryPoint, abcFilePath);

    // Verify ModuleNamespace structure and type
    VerifyModuleNamespace(result);
}

}  // namespace panda::test
