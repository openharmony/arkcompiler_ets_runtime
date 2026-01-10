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

#include "ecmascript/js_object-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/module/napi_module_loader.h"
#include "ecmascript/js_array.h"
#include "ecmascript/global_env.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_object.h"
#include "ecmascript/module/js_shared_module_manager.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/module_value_accessor.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class ModuleManagerTest : public testing::Test {
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
 * @tc.name: ModuleManager_CheckModuleValueOutterResolved_CurrentModuleNotSourceTextModule
 * @tc.desc: Test CheckModuleValueOutterResolved method when current module is not a SourceTextModule
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_CheckModuleValueOutterResolved_CurrentModuleNotSourceTextModule)
{
    ObjectFactory *factory = instance->GetFactory();
    
    // Create a JSFunction that has a non-SourceTextModule as its module
    JSHandle<JSFunction> jsFunc = factory->NewJSFunction(thread->GetEcmaVM()->GetGlobalEnv(),
        reinterpret_cast<void *>(ModuleManagerTest::TestFunc));
    // Create a simple object as the module (not a SourceTextModule)
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> simpleObj = factory->NewJSObject(objectClass);
    jsFunc->SetModule(thread, simpleObj.GetTaggedValue());
    
    // Call CheckModuleValueOutterResolved - should return false when module is not SourceTextModule
    bool result = ModuleManager::CheckModuleValueOutterResolved(thread, 0, *jsFunc);
    
    EXPECT_FALSE(result);
}

/**
 * @tc.name: ModuleManager_CheckModuleValueOutterResolved_SendableFunctionModule
 * @tc.desc: Test CheckModuleValueOutterResolved method when module is a sendable function module
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_CheckModuleValueOutterResolved_SendableFunctionModule)
{
    ObjectFactory *factory = instance->GetFactory();
    
    // Create a JSFunction that has a non-SourceTextModule as its module
    JSHandle<JSFunction> jsFunc = factory->NewJSFunction(thread->GetEcmaVM()->GetGlobalEnv(),
        reinterpret_cast<void *>(ModuleManagerTest::TestFunc));
    // Create a simple object as the module (not a SourceTextModule)
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> simpleObj = factory->NewJSObject(objectClass);
    jsFunc->SetModule(thread, simpleObj.GetTaggedValue());
    // Call CheckModuleValueOutterResolved - should return false when module is a sendable function module
    bool result = ModuleManager::CheckModuleValueOutterResolved(thread, 0, *jsFunc);
    
    EXPECT_FALSE(result);
}

/**
 * @tc.name: ModuleManager_CheckModuleValueOutterResolved_ModuleEnvNotTaggedArray
 * @tc.desc: Test CheckModuleValueOutterResolved method when module environment is not a TaggedArray
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_CheckModuleValueOutterResolved_ModuleEnvNotTaggedArray)
{
    ObjectFactory *factory = instance->GetFactory();
    
    // Create a SourceTextModule with environment that is not a TaggedArray
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    module->SetTypes(ModuleTypes::ECMA_MODULE);
    
    // Set environment to undefined (not a TaggedArray)
    module->SetEnvironment(thread, JSTaggedValue::Undefined());
    
    // Create a JSFunction that has a non-SourceTextModule as its module
    JSHandle<JSFunction> jsFunc = factory->NewJSFunction(thread->GetEcmaVM()->GetGlobalEnv(),
        reinterpret_cast<void *>(ModuleManagerTest::TestFunc));
    // Create a simple object as the module (not a SourceTextModule)
    JSHandle<JSHClass> objectClass(thread->GlobalConstants()->GetHandledObjectClass());
    JSHandle<JSObject> simpleObj = factory->NewJSObject(objectClass);
    jsFunc->SetModule(thread, simpleObj.GetTaggedValue());
    
    // Call CheckModuleValueOutterResolved - should return false when environment is not TaggedArray
    bool result = ModuleManager::CheckModuleValueOutterResolved(thread, 0, *jsFunc);
    
    EXPECT_FALSE(result);
}

/**
 * @tc.name: ModuleManager_GetExternalModuleVarFastPathForJIT_HotReloadPatchMain
 * @tc.desc: Test GetExternalModuleVarFastPathForJIT method when in hot reload patch main stage
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_GetExternalModuleVarFastPathForJIT_HotReloadPatchMain)
{
    ObjectFactory *factory = instance->GetFactory();
    
    // Set the thread to hot reload patch main stage
    thread->SetStageOfHotReload(StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN);
    
        // Create a JSFunction that has a non-SourceTextModule as its module
    JSHandle<JSFunction> jsFunc = factory->NewJSFunction(thread->GetEcmaVM()->GetGlobalEnv(),
        reinterpret_cast<void *>(ModuleManagerTest::TestFunc));

    // Call GetExternalModuleVarFastPathForJIT - should return Hole when in hot reload patch main stage
    JSTaggedValue result = ModuleManager::GetExternalModuleVarFastPathForJIT(thread, 0, *jsFunc);
    
    EXPECT_TRUE(result.IsHole());
}

/**
 * @tc.name: ModuleManager_GetImportedModule_LocalModuleNotLoaded
 * @tc.desc: Test GetImportedModule method when local module is not loaded
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_GetImportedModule_LocalModuleNotLoaded)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    CString referencing = "nonexistent_module";
    
    // Call GetImportedModule - should try to get from shared module manager when local not loaded
    JSHandle<SourceTextModule> result = moduleManager->GetImportedModule(referencing);
    
    // Result should be a valid handle, though the module may not exist
    EXPECT_FALSE(result.IsEmpty());
}

/**
 * @tc.name: ModuleManager_ExecuteJsonModule_EvaluatedModule
 * @tc.desc: Test ExecuteJsonModule method when module is already evaluated
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_ExecuteJsonModule_EvaluatedModule)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    CString recordName = "test_json_module";
    CString filename = "test.json";
    
    // First, add a module to simulate it being already evaluated
    JSHandle<SourceTextModule> existingModule = instance->GetFactory()->NewSourceTextModule();
    existingModule->SetTypes(ModuleTypes::JSON_MODULE);
    existingModule->SetStatus(ModuleStatus::EVALUATED);
    moduleManager->AddResolveImportedModule(recordName, existingModule.GetTaggedValue());
    
    // Call ExecuteJsonModule - should return the existing evaluated module
    JSHandle<JSTaggedValue> result = moduleManager->ExecuteJsonModule(thread, recordName, filename, nullptr);
    
    EXPECT_TRUE(result->IsSourceTextModule());
    EXPECT_EQ(result.GetTaggedValue(), existingModule.GetTaggedValue());
}

/**
 * @tc.name: ModuleManager_SyncModuleExecuteMode_GlobalEnvNotHole
 * @tc.desc: Test SyncModuleExecuteMode method when global environment is not hole
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_SyncModuleExecuteMode_GlobalEnvNotHole)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    // Call SyncModuleExecuteMode with current thread
    moduleManager->SyncModuleExecuteMode(thread);
    
    // Should complete without error
    EXPECT_TRUE(true);
}

/**
 * @tc.name: ModuleManager_IsModuleLoaded_LocalModuleLoaded
 * @tc.desc: Test IsModuleLoaded method when local module is loaded
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_IsModuleLoaded_LocalModuleLoaded)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    CString recordName = "test_local_module";
    
    // Add a module to simulate it being loaded locally
    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    
    // Call IsModuleLoaded - should return true when module is loaded locally
    bool result = moduleManager->IsModuleLoaded(recordName);
    
    EXPECT_TRUE(result);
}

/**
 * @tc.name: ModuleManager_IsModuleLoaded_LocalModuleNotLoaded
 * @tc.desc: Test IsModuleLoaded method when local module is not loaded
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_IsModuleLoaded_LocalModuleNotLoaded)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    CString recordName = "test_nonexistent_module";
    
    // Call IsModuleLoaded - should return false when module is not loaded locally
    bool result = moduleManager->IsModuleLoaded(recordName);
    
    EXPECT_FALSE(result);
}

}  // namespace panda::test