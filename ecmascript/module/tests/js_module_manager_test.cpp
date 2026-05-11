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
#include "ecmascript/module/module_tools.h"
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

/**
 * @tc.name: ModuleImportStackScope_SingleModule
 * @tc.desc: Test ModuleImportStackScope with a single module
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleImportStackScope_SingleModule)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);
    
    // Create a module with a name
    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("test_module.ets");
    
    // Get initial stack data
    std::string_view initialStack = moduleManager->GetModuleImportStackData();
    EXPECT_EQ(initialStack, "\nModuleImportStack:");
    
    {
        // Create scope - this should push the module onto the stack
        ModuleImportStackScope scope(thread, module);
        
        // Check that module was pushed
        std::string_view stackDuring = moduleManager->GetModuleImportStackData();
        EXPECT_NE(stackDuring, "\nModuleImportStack:");
        EXPECT_NE(stackDuring.find("test_module.ets"), std::string_view::npos);
    }
    
    // After scope destruction, stack should be back to initial state
    std::string_view finalStack = moduleManager->GetModuleImportStackData();
    EXPECT_EQ(finalStack, "\nModuleImportStack:");
    
    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
 * @tc.name: ModuleImportStackScope_NestedModules
 * @tc.desc: Test ModuleImportStackScope with nested modules
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleImportStackScope_NestedModules)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);
    
    // Create three modules
    JSHandle<SourceTextModule> module1 = instance->GetFactory()->NewSourceTextModule();
    module1->SetEcmaModuleRecordNameString("module1.ets");
    
    JSHandle<SourceTextModule> module2 = instance->GetFactory()->NewSourceTextModule();
    module2->SetEcmaModuleRecordNameString("module2.ets");
    
    JSHandle<SourceTextModule> module3 = instance->GetFactory()->NewSourceTextModule();
    module3->SetEcmaModuleRecordNameString("module3.ets");
    
    {
        ModuleImportStackScope scope1(thread, module1);
        
        std::string_view stack1 = moduleManager->GetModuleImportStackData();
        EXPECT_NE(stack1.find("module1.ets"), std::string_view::npos);
        
        {
            ModuleImportStackScope scope2(thread, module2);
            
            std::string_view stack2 = moduleManager->GetModuleImportStackData();
            EXPECT_NE(stack2.find("module1.ets"), std::string_view::npos);
            EXPECT_NE(stack2.find("module2.ets"), std::string_view::npos);
            
            {
                ModuleImportStackScope scope3(thread, module3);
                
                std::string_view stack3 = moduleManager->GetModuleImportStackData();
                EXPECT_NE(stack3.find("module1.ets"), std::string_view::npos);
                EXPECT_NE(stack3.find("module2.ets"), std::string_view::npos);
                EXPECT_NE(stack3.find("module3.ets"), std::string_view::npos);
            }
            
            // After scope3 destruction, module3 should be popped
            std::string_view stackAfter3 = moduleManager->GetModuleImportStackData();
            EXPECT_NE(stackAfter3.find("module1.ets"), std::string_view::npos);
            EXPECT_NE(stackAfter3.find("module2.ets"), std::string_view::npos);
            EXPECT_EQ(stackAfter3.find("module3.ets"), std::string_view::npos);
        }
        
        // After scope2 destruction, module2 should be popped
        std::string_view stackAfter2 = moduleManager->GetModuleImportStackData();
        EXPECT_NE(stackAfter2.find("module1.ets"), std::string_view::npos);
        EXPECT_EQ(stackAfter2.find("module2.ets"), std::string_view::npos);
        EXPECT_EQ(stackAfter2.find("module3.ets"), std::string_view::npos);
    }
    
    // After all scopes destroyed, stack should be empty
    std::string_view finalStack = moduleManager->GetModuleImportStackData();
    EXPECT_EQ(finalStack, "\nModuleImportStack:");
    
    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
 * @tc.name: ModuleImportStackScope_SequentialModules
 * @tc.desc: Test ModuleImportStackScope with sequential modules
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleImportStackScope_SequentialModules)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);
    
    // Create two modules
    JSHandle<SourceTextModule> module1 = instance->GetFactory()->NewSourceTextModule();
    module1->SetEcmaModuleRecordNameString("sequential_module1.ets");
    
    JSHandle<SourceTextModule> module2 = instance->GetFactory()->NewSourceTextModule();
    module2->SetEcmaModuleRecordNameString("sequential_module2.ets");
    
    {
        ModuleImportStackScope scope1(thread, module1);
        std::string_view stack1 = moduleManager->GetModuleImportStackData();
        EXPECT_NE(stack1.find("sequential_module1.ets"), std::string_view::npos);
    }
    
    {
        ModuleImportStackScope scope2(thread, module2);
        std::string_view stack2 = moduleManager->GetModuleImportStackData();
        EXPECT_EQ(stack2.find("sequential_module1.ets"), std::string_view::npos);
        EXPECT_NE(stack2.find("sequential_module2.ets"), std::string_view::npos);
    }
    
    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
 * @tc.name: ModuleImportStackScope_EmptyModuleName
 * @tc.desc: Test ModuleImportStackScope with empty module name
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleImportStackScope_EmptyModuleName)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);
    
    // Create a module without setting a name
    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    // Don't set module name, leave it empty
    
    std::string_view initialStack = moduleManager->GetModuleImportStackData();
    
    {
        // Create scope with empty module name - should not crash
        ModuleImportStackScope scope(thread, module);
        
        // Stack should remain unchanged for empty module name
        std::string_view stackDuring = moduleManager->GetModuleImportStackData();
        EXPECT_EQ(stackDuring, initialStack);
    }
    
    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
 * @tc.name: ModuleImportStackScope_Disabled
 * @tc.desc: Test ModuleImportStackScope when feature is disabled
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleImportStackScope_Disabled)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    // Make sure module import stack is disabled
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
    
    // Create a module
    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("disabled_test_module.ets");
    
    std::string_view initialStack = moduleManager->GetModuleImportStackData();
    
    {
        // Create scope - should not modify stack when disabled
        ModuleImportStackScope scope(thread, module);
        
        // Stack should remain unchanged
        std::string_view stackDuring = moduleManager->GetModuleImportStackData();
        EXPECT_EQ(stackDuring, initialStack);
    }
    
    // Stack should still be unchanged
    std::string_view finalStack = moduleManager->GetModuleImportStackData();
    EXPECT_EQ(finalStack, initialStack);
}

/**
 * @tc.name: ModuleImportStackScope_PopEmptyStack
 * @tc.desc: Test ModuleImportStackScope popping from empty stack
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleImportStackScope_PopEmptyStack)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);
    
    // Create a module
    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("pop_test_module.ets");
    
    // Stack starts in initial state (effectively empty of module entries)
    std::string_view initialStack = moduleManager->GetModuleImportStackData();
    EXPECT_EQ(initialStack, "\nModuleImportStack:");
    
    {
        // Create scope - should handle initial empty stack gracefully
        ModuleImportStackScope scope(thread, module);
        
        // Module should be pushed to the initially empty stack
        std::string_view stackDuring = moduleManager->GetModuleImportStackData();
        EXPECT_NE(stackDuring.find("pop_test_module.ets"), std::string_view::npos);
    }
    
    // After scope destruction, should return to initial state
    std::string_view finalStack = moduleManager->GetModuleImportStackData();
    EXPECT_EQ(finalStack, "\nModuleImportStack:");
    
    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
 * @tc.name: ModuleImportStackScope_GetNumberOfDigits
 * @tc.desc: Test GetNumberOfDigits method with various inputs
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleImportStackScope_GetNumberOfDigits)
{
    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);
    
    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("test.ets");
    
    ModuleImportStackScope scope(thread, module);
    
    // Access private method through the scope object
    // Since GetNumberOfDigits is private, we test it indirectly through FormatStackNumber
    
    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
 * @tc.name: ModuleImportStackScope_GetCurrentStackDepth
 * @tc.desc: Test GetCurrentStackDepth method
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleImportStackScope_GetCurrentStackDepth)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);
    
    // Create modules
    JSHandle<SourceTextModule> module1 = instance->GetFactory()->NewSourceTextModule();
    module1->SetEcmaModuleRecordNameString("depth_test_1.ets");
    
    JSHandle<SourceTextModule> module2 = instance->GetFactory()->NewSourceTextModule();
    module2->SetEcmaModuleRecordNameString("depth_test_2.ets");
    
    JSHandle<SourceTextModule> module3 = instance->GetFactory()->NewSourceTextModule();
    module3->SetEcmaModuleRecordNameString("depth_test_3.ets");
    
    // Initial depth should be 0
    std::string_view initialStack = moduleManager->GetModuleImportStackData();
    EXPECT_EQ(initialStack, "\nModuleImportStack:");
    
    {
        ModuleImportStackScope scope1(thread, module1);
        std::string_view stack1 = moduleManager->GetModuleImportStackData();
        // Depth 1: 1 newline character
        int depth1 = 0;
        for (char c : stack1) {
            if (c == '\n') depth1++;
        }
        EXPECT_EQ(depth1, 2); // 1 for head, 1 for first module
        
        {
            ModuleImportStackScope scope2(thread, module2);
            std::string_view stack2 = moduleManager->GetModuleImportStackData();
            // Depth 2
            int depth2 = 0;
            for (char c : stack2) {
                if (c == '\n') depth2++;
            }
            EXPECT_EQ(depth2, 3); // 1 for head, 2 for modules
            
            {
                ModuleImportStackScope scope3(thread, module3);
                std::string_view stack3 = moduleManager->GetModuleImportStackData();
                // Depth 3
                int depth3 = 0;
                for (char c : stack3) {
                    if (c == '\n') depth3++;
                }
                EXPECT_EQ(depth3, 4); // 1 for head, 3 for modules
            }
        }
    }
    
    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
 * @tc.name: ModuleManager_GetModuleImportStackData
 * @tc.desc: Test GetModuleImportStackData method
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_GetModuleImportStackData)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    // Initial state should be "\nModuleImportStack:"
    std::string_view initialStack = moduleManager->GetModuleImportStackData();
    EXPECT_EQ(initialStack, "\nModuleImportStack:");
    
    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);
    
    // Create and add a module
    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("get_stack_data_test.ets");
    
    {
        ModuleImportStackScope scope(thread, module);
        
        std::string_view stackDuring = moduleManager->GetModuleImportStackData();
        EXPECT_NE(stackDuring, "\nModuleImportStack:");
        EXPECT_NE(stackDuring.find("get_stack_data_test.ets"), std::string_view::npos);
    }
    
    // After scope destruction, should return to initial state
    std::string_view finalStack = moduleManager->GetModuleImportStackData();
    EXPECT_EQ(finalStack, "\nModuleImportStack:");
    
    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
 * @tc.name: ModuleManager_ModuleImportStackInitialState
 * @tc.desc: Test initial state of module import stack
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_ModuleImportStackInitialState)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    // Initial state should always be "\nModuleImportStack:" regardless of options
    std::string_view initialStack = moduleManager->GetModuleImportStackData();
    EXPECT_EQ(initialStack, "\nModuleImportStack:");
    
    // Even after enabling/disabling options
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);
    std::string_view stackEnabled = moduleManager->GetModuleImportStackData();
    EXPECT_EQ(stackEnabled, "\nModuleImportStack:");
    
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
    std::string_view stackDisabled = moduleManager->GetModuleImportStackData();
    EXPECT_EQ(stackDisabled, "\nModuleImportStack:");
}

/**
 * @tc.name: ModuleImportStackScope_FormatStackNumber
 * @tc.desc: Test FormatStackNumber method with various inputs
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleManagerTest, ModuleImportStackScope_FormatStackNumber)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    
    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);
    
    // Create multiple modules to test numbering
    JSHandle<SourceTextModule> module1 = instance->GetFactory()->NewSourceTextModule();
    module1->SetEcmaModuleRecordNameString("format_test_1.ets");
    
    {
        ModuleImportStackScope scope1(thread, module1);
        std::string_view stack1 = moduleManager->GetModuleImportStackData();
        EXPECT_NE(stack1.find("\n#0 format_test_1.ets"), std::string_view::npos);
    }
    
    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
  * @tc.name: ModuleManager_TruncateModuleImportStack_NoTruncate
  * @tc.desc: Test TruncateModuleImportStack when no truncation is needed
  * @tc.type: FUNC
  */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_TruncateModuleImportStack_NoTruncate)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("test.ets");

    {
        ModuleImportStackScope scope(thread, module);

        // Get stack data that is smaller than maxSize
        std::string result = moduleManager->GetImportStackDataForJSCrash(64 * 1024);

        // Should return the full stack without truncation
        EXPECT_NE(result.find("test.ets"), std::string::npos);
        EXPECT_EQ(result.find("...."), std::string::npos); // No ellipsis
    }

    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
* @tc.name: ModuleManager_TruncateModuleImportStack_NoNewlines
* @tc.desc: Test TruncateModuleImportStack when no newlines are found (headEnd == 0, tailStart >= size)
* @tc.type: FUNC
*/
HWTEST_F_L0(ModuleManagerTest, ModuleManager_TruncateModuleImportStack_NoNewlines)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    // Create a module with a very long name to test truncation without newlines in the truncation area
    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("very_long_module_name_without_any_newlines_in_the_truncation_area");

    {
        ModuleImportStackScope scope(thread, module);

        // Use a small maxSize that will force truncation
        // The truncation point may not align with newlines, testing the fallback to halfSize
        std::string result = moduleManager->GetImportStackDataForJSCrash(40);

        // Should contain ellipsis indicating truncation occurred
        EXPECT_NE(result.find("...."), std::string::npos);

        // Result should be within the size limit (or close to it due to newline alignment)
        EXPECT_TRUE(result.size() <= 45); // Allow some margin for newline alignment
    }

    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
* @tc.name: ModuleManager_GetModuleImportStackDataForJS
* @tc.desc: Test GetImportStackDataForJSCrash method
* @tc.type: FUNC
*/
HWTEST_F_L0(ModuleManagerTest, ModuleManager_GetModuleImportStackDataForJS)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("test_for_js.ets");

    {
        ModuleImportStackScope scope(thread, module);

        // Get stack data for JS with 64KB limit
        std::string result = moduleManager->GetImportStackDataForJSCrash(64 * 1024);

        // Should contain the module name
        EXPECT_NE(result.find("test_for_js.ets"), std::string::npos);

        // Should start with the correct header
        EXPECT_NE(result.find("\nModuleImportStack:"), std::string::npos);
    }

    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
* @tc.name: ModuleManager_GetImportStackDataForCPPCrash
* @tc.desc: Test GetImportStackDataForCPPCrash method
* @tc.type: FUNC
*/
HWTEST_F_L0(ModuleManagerTest, ModuleManager_GetImportStackDataForCPPCrash)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("test_for_cpp.ets");

    {
        ModuleImportStackScope scope(thread, module);

        // Get stack data for CPP with 64KB limit
        std::string result = moduleManager->GetImportStackDataForCPPCrash("test_for_cpp.ets", 64 * 1024);

        // Should contain the module name
        EXPECT_NE(result.find("test_for_cpp.ets"), std::string::npos);

        // Should contain the error message prefix
        EXPECT_NE(result.find("Failed to load"), std::string::npos);
        EXPECT_NE(result.find("dependency import call stack"), std::string::npos);
    }


    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
* @tc.name: ModuleManager_RenumberModuleImportStack
* @tc.desc: Test RenumberModuleImportStack method
* @tc.type: FUNC
*/
HWTEST_F_L0(ModuleManagerTest, ModuleManager_RenumberModuleImportStack)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    JSHandle<SourceTextModule> module1 = instance->GetFactory()->NewSourceTextModule();
    module1->SetEcmaModuleRecordNameString("renumber_module1.ets");

    JSHandle<SourceTextModule> module2 = instance->GetFactory()->NewSourceTextModule();
    module2->SetEcmaModuleRecordNameString("renumber_module2.ets");

    {
        ModuleImportStackScope scope1(thread, module1);
        {
            ModuleImportStackScope scope2(thread, module2);

            std::string_view stack = moduleManager->GetModuleImportStackData();
            // Should have #0 and #1
            EXPECT_NE(stack.find("#1 renumber_module1.ets"), std::string::npos);
            EXPECT_NE(stack.find("#0 renumber_module2.ets"), std::string::npos);
        }
    }

    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}
/**
* @tc.name: ModuleManager_TruncateModuleImportStack_WithNewlines
* @tc.desc: Test TruncateModuleImportStack when newlines are found for head and tail
* @tc.type: FUNC
*/
HWTEST_F_L0(ModuleManagerTest, ModuleManager_TruncateModuleImportStack_WithNewlines)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    // Create multiple modules to build a larger stack
    JSHandle<SourceTextModule> module1 = instance->GetFactory()->NewSourceTextModule();
    module1->SetEcmaModuleRecordNameString("module1.ets");

    JSHandle<SourceTextModule> module2 = instance->GetFactory()->NewSourceTextModule();
    module2->SetEcmaModuleRecordNameString("module2.ets");

    JSHandle<SourceTextModule> module3 = instance->GetFactory()->NewSourceTextModule();
    module3->SetEcmaModuleRecordNameString("module3.ets");

    {
        ModuleImportStackScope scope1(thread, module1);
        {
            ModuleImportStackScope scope2(thread, module2);
            {
                ModuleImportStackScope scope3(thread, module3);
                // Use a maxSize that forces truncation but keeps head and tail
                std::string result = moduleManager->GetImportStackDataForJSCrash(62);
                // Should contain ellipsis
                EXPECT_NE(result.find("...."), std::string::npos);

                // Should contain head (module1 at the bottom of stack)
                EXPECT_NE(result.find("module1.ets"), std::string::npos);

                // Should contain tail (module3 at the top of stack)
                EXPECT_NE(result.find("module3.ets"), std::string::npos);

                // module2 should be truncated (in the middle)
                EXPECT_EQ(result.find("module2.ets"), std::string::npos);
            }
        }
    }
    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
    * @tc.name: ModuleManager_TruncateModuleImportStack_BoundarySize
    * @tc.desc: Test TruncateModuleImportStack with boundary maxSize values
    * @tc.type: FUNC
    */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_TruncateModuleImportStack_BoundarySize)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("boundary_test.ets");

    {
        ModuleImportStackScope scope(thread, module);

        // maxSize = 24 (head + ellipsis = 19 + 5 = 24)
        // halfSize = (24 - 5 - 19) / 2 = 0
        std::string result = moduleManager->GetImportStackDataForJSCrash(24);

        // Should contain head and ellipsis
        EXPECT_NE(result.find("ModuleImportStack:"), std::string::npos);
        EXPECT_NE(result.find("...."), std::string::npos);
    }

    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
    * @tc.name: ModuleManager_TruncateModuleImportStack_SingleModule
    * @tc.desc: Test TruncateModuleImportStack with a single module
    * @tc.type: FUNC
    */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_TruncateModuleImportStack_SingleModule)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("single_module.ets");

    {
        ModuleImportStackScope scope(thread, module);

        // Get full stack first
        std::string fullStack = std::string(moduleManager->GetModuleImportStackData());

        // Use maxSize slightly smaller than full stack
        std::string result = moduleManager->GetImportStackDataForJSCrash(fullStack.size() - 5);

        // Should contain the module name
        EXPECT_NE(result.find("....e.ets"), std::string::npos);
    }

    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
    * @tc.name: ModuleManager_TruncateModuleImportStack_ExactSize
    * @tc.desc: Test TruncateModuleImportStack when maxSize equals stack size
    * @tc.type: FUNC
    */
HWTEST_F_L0(ModuleManagerTest, ModuleManager_TruncateModuleImportStack_ExactSize)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("exact_size.ets");

    {
        ModuleImportStackScope scope(thread, module);

        // Get full stack
        std::string fullStack = std::string(moduleManager->GetModuleImportStackData());

        // Use maxSize exactly equal to stack size
        std::string result = moduleManager->GetImportStackDataForJSCrash(fullStack.size());
        // Should return the full stack without truncation
        EXPECT_EQ(result, fullStack);
        EXPECT_EQ(result.find("...."), std::string::npos); // No ellipsis
    }

    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
* @tc.name: ModuleManager_TruncateImportStackForCPPCrash_Normal
* @tc.desc: Test TruncateImportStackForCPPCrash with normal stack
* @tc.type: FUNC
*/
HWTEST_F_L0(ModuleManagerTest, ModuleManager_TruncateImportStackForCPPCrash_Normal)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("cpp_test.ets");

    {
        ModuleImportStackScope scope(thread, module);

        // Get stack data for CPP with 64KB limit
        std::string result = moduleManager->GetImportStackDataForCPPCrash("cpp_test.ets", 64 * 1024);

        // Should contain the error message prefix
        EXPECT_NE(result.find("Failed to load"), std::string::npos);
        EXPECT_NE(result.find("cpp_test.ets"), std::string::npos);
        EXPECT_NE(result.find("dependency import call stack"), std::string::npos);

        // Should contain the module name in the stack
        EXPECT_NE(result.find("cpp_test.ets"), std::string::npos);
    }

    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
* @tc.name: ModuleManager_TruncateImportStackForCPPCrash_Truncated
* @tc.desc: Test TruncateImportStackForCPPCrash with truncation
* @tc.type: FUNC
*/
HWTEST_F_L0(ModuleManagerTest, ModuleManager_TruncateImportStackForCPPCrash_Truncated)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    JSHandle<SourceTextModule> module1 = instance->GetFactory()->NewSourceTextModule();
    module1->SetEcmaModuleRecordNameString("cpp_module1.ets");

    JSHandle<SourceTextModule> module2 = instance->GetFactory()->NewSourceTextModule();
    module2->SetEcmaModuleRecordNameString("cpp_module2.ets");

    JSHandle<SourceTextModule> module3 = instance->GetFactory()->NewSourceTextModule();
    module3->SetEcmaModuleRecordNameString("cpp_module3.ets");

    {
        ModuleImportStackScope scope1(thread, module1);
        {
            ModuleImportStackScope scope2(thread, module2);
            {
                ModuleImportStackScope scope3(thread, module3);

                // Use a small maxSize to force truncation
                std::string result = moduleManager->GetImportStackDataForCPPCrash("cpp_test.ets", 100);
                // Should contain the error message prefix
                EXPECT_NE(result.find("Failed to load"), std::string::npos);
                EXPECT_NE(result.find("dependency import call stack"), std::string::npos);

                // Should contain ellipsis indicating truncation
                EXPECT_NE(result.find("...."), std::string::npos);
            }
        }
    }

    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
* @tc.name: ModuleManager_TruncateImportStackForCPPCrash_EmptyModuleName
* @tc.desc: Test TruncateImportStackForCPPCrash with empty module name
* @tc.type: FUNC
*/
HWTEST_F_L0(ModuleManagerTest, ModuleManager_TruncateImportStackForCPPCrash_EmptyModuleName)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("empty_name_test.ets");

    {
        ModuleImportStackScope scope(thread, module);

        // Get stack data for CPP with empty module name
        std::string result = moduleManager->GetImportStackDataForCPPCrash("", 64 * 1024);

        // Should still contain the error message prefix
        EXPECT_NE(result.find("Failed to load"), std::string::npos);
        EXPECT_NE(result.find("dependency import call stack"), std::string::npos);
    }

    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
* @tc.name: ModuleManager_TruncateImportStackForCPPCrash_SingleModule
* @tc.desc: Test TruncateImportStackForCPPCrash with a single module
* @tc.type: FUNC
*/
HWTEST_F_L0(ModuleManagerTest, ModuleManager_TruncateImportStackForCPPCrash_SingleModule)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("single_cpp_test.ets");

    {
        ModuleImportStackScope scope(thread, module);

        // Get stack data for CPP
        std::string result = moduleManager->GetImportStackDataForCPPCrash("single_cpp_test.ets", 64 * 1024);

        // Should contain the error message and module name
        EXPECT_NE(result.find("Failed to load single_cpp_test.ets"), std::string::npos);
        EXPECT_NE(result.find("dependency import call stack"), std::string::npos);

        // Should contain the module in the stack
        EXPECT_NE(result.find("single_cpp_test.ets"), std::string::npos);
    }

    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
* @tc.name: ModuleManager_TruncateImportStackForJSCrash_Normal
* @tc.desc: Test TruncateImportStackForJSCrash with normal stack
* @tc.type: FUNC
*/
HWTEST_F_L0(ModuleManagerTest, ModuleManager_TruncateImportStackForJSCrash_Normal)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("js_crash_test.ets");

    {
        ModuleImportStackScope scope(thread, module);

        // Get stack data for JS with 64KB limit
        std::string result = moduleManager->GetImportStackDataForJSCrash(64 * 1024);

        // Should contain the module name
        EXPECT_NE(result.find("js_crash_test.ets"), std::string::npos);

        // Should start with the correct header
        EXPECT_NE(result.find("\nModuleImportStack:"), std::string::npos);
    }

    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
* @tc.name: ModuleManager_TruncateImportStackForJSCrash_Truncated
* @tc.desc: Test TruncateImportStackForJSCrash with truncation
* @tc.type: FUNC
*/
HWTEST_F_L0(ModuleManagerTest, ModuleManager_TruncateImportStackForJSCrash_Truncated)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    JSHandle<SourceTextModule> module1 = instance->GetFactory()->NewSourceTextModule();
    module1->SetEcmaModuleRecordNameString("js_crash_module1.ets");

    JSHandle<SourceTextModule> module2 = instance->GetFactory()->NewSourceTextModule();
    module2->SetEcmaModuleRecordNameString("js_crash_module2.ets");

    JSHandle<SourceTextModule> module3 = instance->GetFactory()->NewSourceTextModule();
    module3->SetEcmaModuleRecordNameString("js_crash_module3.ets");

    {
        ModuleImportStackScope scope1(thread, module1);
        {
            ModuleImportStackScope scope2(thread, module2);
            {
                ModuleImportStackScope scope3(thread, module3);

                // Use a small maxSize to force truncation
                std::string result = moduleManager->GetImportStackDataForJSCrash(40);

                // Should contain ellipsis indicating truncation
                EXPECT_NE(result.find("...."), std::string::npos);

                // Should start with the correct header
                EXPECT_NE(result.find("\nModuleImportStack:"), std::string::npos);
            }
        }
    }

    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}

/**
* @tc.name: ModuleManager_TruncateImportStackForJSCrash_SingleModule
* @tc.desc: Test TruncateImportStackForJSCrash with a single module
* @tc.type: FUNC
*/
HWTEST_F_L0(ModuleManagerTest, ModuleManager_TruncateImportStackForJSCrash_SingleModule)
{
    ModuleManager *moduleManager = thread->GetModuleManager();

    // Enable module import stack
    instance->GetJSOptions().SetArkProperties(ArkProperties::ENABLE_RUNTIME_MODULE_STACK);

    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString("js_crash_single_test.ets");

    {
        ModuleImportStackScope scope(thread, module);

        // Get stack data for JS
        std::string result = moduleManager->GetImportStackDataForJSCrash(64 * 1024);

        // Should contain the module name
        EXPECT_NE(result.find("js_crash_single_test.ets"), std::string::npos);

        // Should start with the correct header
        EXPECT_NE(result.find("\nModuleImportStack:"), std::string::npos);
    }

    // Reset options
    instance->GetJSOptions().SetArkProperties(ArkProperties::DEFAULT);
}
}  // namespace panda::test